#!/usr/bin/env python3
"""InfInt and UInt oracle suite: quick smoke, full feature coverage, extended seeded stress.

Every mode builds the optimized scalar, checked scalar and (when the CPU has them)
AVX2/FMA/BMI2 configurations of the C++ oracle with a second translation unit and runs
the self-test phases (schoolbook limb products, identities on oracle products,
two's-complement bit models, sieves, native 128-bit arithmetic, libstdc++'s integer std::format)
and the differential protocol against Python's big integers, format() and base conversion (cases are generated and validated as a
stream, so the corpus never sits in memory); full and stress add ASan/UBSan scalar and AVX2 builds with INFINT_SCRATCH_HEAP (every
arena frame a separate heap block) and the checked build runs the --invalid precondition probes; stress repeats
--rounds times with consecutive seeds and larger operands. Every check survives -DNDEBUG.
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
SECOND = SOURCE.with_name("07-infint_tester_tu.cpp")
ROOT = SOURCE.parents[2]
INVALID_CASES = ("div-zero", "rem-zero", "divmod-zero", "divmodslow-zero", "scalar-div-zero", "divexact-zero", "divexact-inexact",
                 "divfloor-inf", "shift-negative", "sqrt-negative", "root-even-negative", "root-zero-k", "jacobi-even", "inv-zero-mod",
                 "powmod-zero-mod", "pow-negative", "read-base", "tostring-base", "todigits-base", "ctor-string", "ctor-digits",
                 "fromlimbs-sign", "fromdouble-nan", "bitwise-inf", "testbit-negative-index", "popcount-inf", "sample-zero",
                 "removefactor-one", "newton-unnormalized", "uint-shift", "uint-divword", "uint-div-zero", "uint-from-inf",
                 "isprime-inf", "tolimbs-inf", "tochars-base", "charsbound-base", "rshift-negative", "setbit-negative",
                 "clearbit-negative", "scan0-negative", "scan1-negative", "not-inf", "ctz-inf", "bitlength-inf", "issquare-inf",
                 "kronecker-inf", "todigits-inf", "root-inf", "uint-rshift", "removefactor-zero", "uint-ctor-string", "bitlength-words",
                 "popcount-words")
DIGITS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
FILLS = " *0#+-x<^_~"


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


def execute(command: list[str], label: str, timeout: int, env: dict[str, str],
            expected_death: bool = False, stdin: str | None = None) -> str:
    try:
        result = subprocess.run(command, cwd="/tmp", env=env, text=True, input=stdin,
                                capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired as error:
        raise Failure(f"{label}: timeout={timeout}s\ncommand={shlex.join(command)}\n"
                      f"stdout={str(error.stdout)[-2000:]!r}\nstderr={str(error.stderr)[-2000:]!r}") from error
    if expected_death:
        okay = result.returncode == -signal.SIGABRT and "assert" in result.stderr.lower()
    else:
        okay = result.returncode == 0
    if not okay:
        raise Failure(f"{label}: exit={result.returncode} expected="
                      f"{'assertion/SIGABRT' if expected_death else '0'}\n"
                      f"command={shlex.join(command)}\n"
                      f"stdout:\n{result.stdout[-3000:]}\nstderr:\n{result.stderr[-3000:]}")
    return result.stdout


def to_base(a: int, base: int) -> str:
    if a == 0:
        return "0"
    neg, a = a < 0, abs(a)
    out = []
    while a:
        a, d = divmod(a, base)
        out.append(DIGITS[d])
    return ("-" if neg else "") + "".join(reversed(out))


def check_tochars(line: str, a: int, base: int, lower: int) -> None:
    """toChars text against to_base and charsBound (the second field) against the length: an upper bound, exact for power-of-two
    bases (digits plus a sign slot)."""
    text, _, bound = line.rpartition(" ")
    expected = to_base(a, base).lower() if lower else to_base(a, base)
    if text != expected:
        shown = lambda x: x if len(x) <= 200 else x[:90] + "..." + x[-90:]
        raise Failure(f"toChars text: expected {shown(expected)!r}, got {shown(text)!r}")
    if int(bound) < len(text) or (base & (base - 1) == 0 and int(bound) != len(text) + (a >= 0)):
        raise Failure(f"charsBound {bound} for a text of length {len(text)} in base {base}")


def random_spec(rng: random.Random) -> tuple[str, str, str, str, str, str, str]:
    """[[fill]align][sign][#][0][width][type] parts with a literal width of 1-60 or none and the types d b B o x X."""
    kind = rng.randrange(3)
    align = rng.choice("<>^") if kind else ""
    fill = rng.choice(FILLS) if kind == 2 else ""
    return (fill, align, rng.choice(["", "", "+", "-", " "]), rng.choice(["", "#"]), rng.choice(["", "", "0"]),
            str(rng.randrange(1, 61)) if rng.randrange(3) else "", rng.choice(["", "d", "b", "B", "o", "x", "X"]))


def cpp_format(v: int, fill: str, align: str, sign: str, alt: str, zero: str, width: str, typ: str) -> str:
    """std::format of an integer through Python's format(): C++ ignores 0 beside an explicit alignment (Python turns it into the fill),
    prints B as Python's b with the prefix 0B, and prefixes octal with 0 for nonzero values (Python writes 0o), so #o pads the
    sign, prefix and digits as one string (or zero-pads the digits behind the sign)."""
    if align:
        zero = ""
    if typ == "o" and alt:
        sgn = "-" if v < 0 else ("" if sign in ("", "-") else sign)
        body = ("0" if v else "") + format(abs(v), "o")
        if zero:
            return sgn + format(body, "0>" + str(max(int(width or 0) - len(sgn), 0)))
        return format(sgn + body, (fill or " ") + (align or ">") + width)
    out = format(v, fill + align + sign + alt + zero + width + ("b" if typ == "B" else typ))
    return out.replace("0b", "0B", 1) if typ == "B" and alt else out


def digits_of(a: int, base: int) -> list[int]:
    a = abs(a)
    if a == 0:
        return [0]
    out = []
    while a:
        a, d = divmod(a, base)
        out.append(d)
    return out[::-1]


def kronecker(a: int, n: int) -> int:
    if n == 0:
        return 1 if abs(a) == 1 else 0
    res = 1
    if n < 0:
        n = -n
        if a < 0:
            res = -1
    t = 0
    while n % 2 == 0:
        n //= 2
        t += 1
    if t:
        if a % 2 == 0:
            return 0
        if t % 2 == 1 and a % 8 in (3, 5):
            res = -res
    return res * jacobi(a, n)


def jacobi(a: int, n: int) -> int:
    a %= n
    res = 1
    while a:
        while a % 2 == 0:
            a //= 2
            if n % 8 in (3, 5):
                res = -res
        a, n = n, a
        if a % 4 == 3 and n % 4 == 3:
            res = -res
        a %= n
    return res if n == 1 else 0


def is_prime(n: int) -> bool:
    if n < 2:
        return False
    for p in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        if n % p == 0:
            return n == p
    d, s = n - 1, 0
    while d % 2 == 0:
        d //= 2
        s += 1
    bases = (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71)
    for a in bases:
        x = pow(a, d, n)
        if x in (1, n - 1):
            continue
        for _ in range(s - 1):
            x = x * x % n
            if x == n - 1:
                break
        else:
            return False
    return True


def iroot(a: int, k: int) -> int:
    if a < 0:
        return -iroot(-a, k)
    if a < 2:
        return a
    x = 1 << ((a.bit_length() + k - 1) // k)
    while True:
        y = ((k - 1) * x + a // x ** (k - 1)) // k
        if y >= x:
            return x
        x = y


def is_perfect_power(a: int) -> bool:
    m = abs(a)
    if m <= 1:
        return True
    for p in range(2, m.bit_length() + 1):
        if a < 0 and p % 2 == 0:
            continue
        r = iroot(m, p)
        if r ** p == m:
            return True
    return False


def rand_bits(rng: random.Random, bits: int, signed: bool = False) -> int:
    if bits <= 0:
        return 0
    x = (1 << (bits - 1)) | rng.getrandbits(bits - 1)
    return -x if signed and rng.randrange(2) else x


def build_cases(seed: int, mode: str):
    """Yields (command, description, checker) lazily so the corpus never sits in memory; a checker is an expected string or a
    callable raising Failure."""
    rng = random.Random(seed)
    cases: list[tuple[str, str, object]] = []
    count = {"quick": 60, "full": 300, "stress": 900}[mode]

    def add(command: str, description: str, expected) -> None:
        cases.append((command, description, str(expected) if not callable(expected) else expected))

    def trunc(a: int, b: int) -> tuple[int, int]:
        q = abs(a) // abs(b)
        if (a < 0) != (b < 0):
            q = -q
        return q, a - q * b

    bits_choices = [1, 2, 31, 32, 33, 63, 64, 65, 127, 128, 129, 255, 256, 257, 511, 1024, 1025, 2047, 4096, 8191]
    big_choices = [20000, 32768, 65537, 100000, 150000] if mode != "quick" else [4000]
    add("tochars 16 1 inf", "toChars sentinel", "inf 4")
    add("tochars 10 0 -inf", "toChars negative sentinel", "-inf 4")
    for base in (2, 10, 36):
        cases.append((f"tochars {base} 0 0", f"toChars zero base {base}", lambda line, base=base: check_tochars(line, 0, base, 0)))
    yield from cases; cases.clear()
    for t in range({"quick": 300, "full": 3000, "stress": 9000}[mode]):
        v = 0 if t % 25 == 0 else rand_bits(rng, rng.randrange(1, 301), True)
        parts = random_spec(rng)
        spec = "".join(parts)
        add(f"format {v} {spec}", f"format {{:{spec}}} #{t}", cpp_format(v, *parts))
        if t % 100 == 99:
            yield from cases; cases.clear()
    yield from cases; cases.clear()
    for t in range(count):
        a = rand_bits(rng, rng.choice(bits_choices), True)
        b = rand_bits(rng, rng.choice(bits_choices), True)
        if t % 10 == 0:
            a = rand_bits(rng, rng.choice(big_choices), True)
        if t % 15 == 0:
            b = rand_bits(rng, rng.choice(big_choices), True)
        if t % 17 == 0:
            a = (1 << rng.choice(bits_choices)) - 1
        add(f"add {a} {b}", f"add #{t}", a + b)
        add(f"sub {a} {b}", f"sub #{t}", a - b)
        add(f"mul {a} {b}", f"mul #{t}", a * b)
        add(f"and {a} {b}", f"and #{t}", a & b)
        add(f"or {a} {b}", f"or #{t}", a | b)
        add(f"xor {a} {b}", f"xor #{t}", a ^ b)
        add(f"not {a}", f"not #{t}", ~a)
        add(f"cmp {a} {b}", f"cmp #{t}", (a > b) - (a < b))
        add(f"neg {a}", f"neg #{t}", -a)
        add(f"inc {a}", f"inc #{t}", a + 1)
        add(f"dec {a}", f"dec #{t}", a - 1)
        k = rng.randrange(1, 7)
        edge = rng.choice([1, -1]) * ((1 << (64 * k)) - rng.randrange(0, 3))
        for x, y, z in ((a, b, edge), (edge, rng.choice([1, -1, edge, -edge, 2]), b), (edge, a, -edge)):
            add(f"addchain {x} {y} {z}", f"addchain ({k}-word edge) #{t}", f"{(x + y) + z} {(x - y) - z} {x + (y - z)} {x - y}")
        if b:
            q, r = trunc(a, b)
            add(f"div {a} {b}", f"divMod #{t}", f"{q} {r}")
            add(f"divfloor {a} {b}", f"divModFloor #{t}", f"{a // b} {a - (a // b) * b}")
            cq = -((-a) // b)
            add(f"divceil {a} {b}", f"divModCeil #{t}", f"{cq} {a - cq * b}")
            er = a % abs(b)
            add(f"diveuclid {a} {b}", f"divModEuclid #{t}", f"{(a - er) // b} {er}")
            add(f"divexact {a * b} {b}", f"divExact #{t}", a)
            if abs(a) < 1 << 20000 and abs(b) < 1 << 20000:
                add(f"divslow {a} {b}", f"divModSlow #{t}", f"{q} {r}")
            g = math.gcd(a, b)
            add(f"gcd {a} {b}", f"gcd #{t}", g)
            add(f"lcm {a} {b}", f"lcm #{t}", abs(a * b) // g if a else 0)

            def check_exgcd(line: str, a: int = a, b: int = b, g: int = g) -> None:
                gg, x, y = map(int, line.split())
                if gg != g or a * x + b * y != g:
                    raise Failure(f"exGcd identity fails: {line}")
                if a and b and (abs(x) > abs(b) // g or abs(y) > abs(a) // g):
                    raise Failure(f"exGcd cofactor bound fails: {line}")
            cases.append((f"exgcd {a} {b}", f"exGcd #{t}", check_exgcd))
            m = abs(b)
            if m > 1:
                expected_inv = pow(a, -1, m) if math.gcd(a, m) == 1 else -1
                add(f"inv {a} {b}", f"inv #{t}", expected_inv)
            else:
                add(f"inv {a} {b}", f"inv mod 1 #{t}", 0)
            if m % 2 == 1:
                add(f"jacobi {a} {m}", f"jacobi #{t}", jacobi(a, m))
            add(f"kronecker {a} {b}", f"kronecker #{t}", kronecker(a, b))
            f = abs(b) if abs(b) >= 2 else 3
            k = rng.randrange(5)
            x = a if a else 1
            if x % f:
                add(f"removefactor {x * f ** k} {f}", f"removeFactor #{t}", f"{k} {x}")
        s = rng.randrange(0, 3000)
        add(f"shl {a} {s}", f"shl #{t}", a << s)
        add(f"shr {a} {s}", f"shr #{t}", a >> s)
        add(f"bitlength {a}", f"bitLength #{t}", abs(a).bit_length())
        add(f"ctz {a}", f"ctz #{t}", 0 if a == 0 else (abs(a) & -abs(a)).bit_length() - 1)
        add(f"popcount {a}", f"popcount #{t}", abs(a).bit_count())
        i = rng.randrange(0, 2200)
        add(f"testbit {a} {i}", f"testBit #{t}", int((a >> i) & 1))
        add(f"setbit {a} {i}", f"setBit #{t}", a | (1 << i))
        add(f"clearbit {a} {i}", f"clearBit #{t}", a & ~(1 << i))
        add(f"flipbit {a} {i}", f"flipBit #{t}", a ^ (1 << i))
        j1 = next((j for j in range(i, i + 70000) if (a >> j) & 1), -1) if a < 0 or (a >> i) else -1
        if a >= 0 and (a >> i):
            j1 = i + ((a >> i) & -(a >> i)).bit_length() - 1
        j0 = -1
        if a >= 0:
            j0 = i + ((~a >> i) & -(~a >> i)).bit_length() - 1
        else:
            inv = ~a
            j0 = i + ((inv >> i) & -(inv >> i)).bit_length() - 1 if (inv >> i) else -1
        add(f"scan1 {a} {i}", f"scan1 #{t}", j1)
        add(f"scan0 {a} {i}", f"scan0 #{t}", j0)
        e = rng.randrange(0, 40)
        if abs(a) < 1 << 2000:
            add(f"pow {a} {e}", f"pow #{t}", a ** e)
        k = rng.choice([1, 2, 3, 5, 7, 16, 31])
        if a >= 0 or k % 2 == 1:
            root = iroot(a, k)
            add(f"kthroot {a} {k}", f"kthRootRem #{t}", f"{root} {a - root ** k}")
        if a >= 0:
            root = math.isqrt(a)
            add(f"sqrt {a}", f"isqrtRem #{t}", f"{root} {a - root * root}")
            add(f"issquare {a} ", f"isPerfectSquare #{t}", int(root * root == a))
            add(f"issquare {a * a}", f"isPerfectSquare of square #{t}", 1)
        if abs(a) < 1 << 600:
            add(f"ispower {a}", f"isPerfectPower #{t}", int(is_perfect_power(a)))
            e2 = rng.randrange(2, 8)
            if e2 % 2 == 1 or a >= 0:
                add(f"ispower {a ** e2}", f"isPerfectPower of power #{t}", 1)
        base = rng.choice([2, 3, 7, 8, 10, 16, 32, 36])
        add(f"tobase {a} {base}", f"toString base {base} #{t}", to_base(a, base))
        add(f"frombase {base} {to_base(a, base)}", f"read base {base} #{t}", a)
        add(f"frombase {base} {to_base(a, base).lower()}", f"read lowercase base {base} #{t}", a)
        for cbase, lower in ((rng.randrange(2, 37), rng.randrange(2)), (rng.choice([2, 4, 8, 16, 32]), rng.randrange(2))):
            cases.append((f"tochars {cbase} {lower} {a}", f"toChars base {cbase}{' lower' if lower else ''} #{t}",
                          lambda line, a=a, cbase=cbase, lower=lower: check_tochars(line, a, cbase, lower)))
        dbase = rng.choice([2, 10, 1000, 65536, 1000000007, 4294967295])
        if abs(a) < 1 << 5000:
            digits = digits_of(a, dbase)
            add(f"todigits {abs(a)} {dbase}", f"toDigits base {dbase} #{t}", " ".join(map(str, digits)))
            add(f"fromdigits {dbase} {len(digits)} " + " ".join(map(str, digits)), f"digit constructor base {dbase} #{t}", abs(a))
        add(f"todouble {a}", f"toDouble #{t}", lambda line, a=a: check_double(line, a))
        sc = rng.choice([1, -1, 3, -7, 10, 2 ** 31, 2 ** 32 - 1, 2 ** 32, 2 ** 63 - 1, -(2 ** 63), 2 ** 40 + 1])
        add(f"muls {a} {sc}", f"scalar product #{t}", a * sc)
        q, r = trunc(a, sc)
        add(f"divs {a} {sc}", f"scalar division #{t}", f"{q} {r} {q} {r}")
        for ty, lo, hi in (("int", -2 ** 31, 2 ** 31 - 1), ("lng", -2 ** 63, 2 ** 63 - 1), ("ulng", 0, 2 ** 64 - 1), ("lll", -2 ** 127, 2 ** 127 - 1), ("ulll", 0, 2 ** 128 - 1)):
            v = rng.choice([a, lo, hi, lo - 1, hi + 1, 0, -1])
            add(f"trycast {ty} {v}", f"tryCast {ty} #{t}", f"1 {v}" if lo <= v <= hi else "0")
        add(f"hash {a}", f"hash #{t}", lambda line, a=a: None)
        limbs = []
        m = abs(a)
        while m:
            limbs.append(m & 0xFFFFFFFF)
            m >>= 32
        words = []
        m = abs(a)
        while m:
            words.append(m & (2 ** 64 - 1))
            m >>= 64
        add(f"limbs {a}", f"limbs #{t}", " ".join(map(str, limbs or [0])) + " |" + "".join(f" {w}" for w in (words or [0])) + f" | {a} {a}")
        yield from cases; cases.clear()
    yield from cases; cases.clear()
    for t in range(count // 3):
        m = rand_bits(rng, rng.choice([2, 33, 64, 65, 127, 128, 129, 300, 1024, 1025, 4100]) if mode != "quick" else rng.choice([2, 64, 65, 300]))
        if t % 2:
            m |= 1
        a = rand_bits(rng, rng.randrange(1, m.bit_length() + 40), True)
        e = rng.getrandbits(rng.randrange(1, 2500 if mode != "quick" else 400))
        add(f"powmod {a} {e} {m}", f"powMod #{t}", pow(a, e, m))
        add(f"powmod {a} {e} {-m}", f"powMod negative modulus #{t}", pow(a, e, m))
        if math.gcd(a, m) == 1 and m > 1:
            add(f"powmod {a} {-e} {m}", f"powMod negative exponent #{t}", pow(a, -e, m))
        else:
            add(f"powmod {a} {-e} {m}", f"powMod no inverse #{t}", 0 if m == 1 else (-1 if e else 1))
        p = rand_bits(rng, rng.choice([70, 128, 200, 512]))
        p |= 1
        while not is_prime(p):
            p += 2
        add(f"isprime {p}", f"isProbablePrime prime #{t}", 1)
        q = rand_bits(rng, rng.choice([70, 128, 200]))
        q |= 1
        while not is_prime(q):
            q += 2
        add(f"isprime {p * q}", f"isProbablePrime semiprime #{t}", 0)
        add(f"isprime {p * p}", f"isProbablePrime square #{t}", 0)
        c = rand_bits(rng, rng.choice([80, 200, 700]))
        add(f"isprime {c}", f"isProbablePrime random #{t}", int(is_prime(c)))
        yield from cases; cases.clear()
    yield from cases; cases.clear()
    for t in range(count // 2):
        n = rand_bits(rng, rng.choice([1, 63, 64, 65, 100, 200, 256, 300]), True)
        m = rand_bits(rng, rng.choice([1, 2, 64, 65, 128, 255, 256, 1000]), True)
        mod = 1 << 256
        x, y = n % mod, m % mod
        add(f"u256 add {n} {m}", f"u256 add #{t}", (x + y) % mod)
        add(f"u256 sub {n} {m}", f"u256 sub #{t}", (x - y) % mod)
        add(f"u256 mul {n} {m}", f"u256 mul #{t}", (x * y) % mod)
        add(f"u256 mulhigh {n} {m}", f"u256 mulHigh #{t}", (x * y) >> 256)
        add(f"u256 and {n} {m}", f"u256 and #{t}", x & y)
        add(f"u256 or {n} {m}", f"u256 or #{t}", x | y)
        add(f"u256 xor {n} {m}", f"u256 xor #{t}", x ^ y)
        add(f"u256 not {n} {m}", f"u256 not #{t}", (~x) % mod)
        add(f"u256 cmp {n} {m}", f"u256 cmp #{t}", (x > y) - (x < y))
        if y:
            add(f"u256 div {n} {m}", f"u256 div #{t}", x // y)
            add(f"u256 mod {n} {m}", f"u256 mod #{t}", x % y)
        s = rng.randrange(0, 256)
        add(f"u256 shl {n} {s}", f"u256 shl #{t}", (x << s) % mod)
        add(f"u256 shr {n} {s}", f"u256 shr #{t}", x >> s)
        d = rng.getrandbits(64) | 1
        add(f"u256 divword {n} {d}", f"u256 divWord #{t}", f"{x // d} {x % d}")
        base = rng.randrange(2, 37)
        add(f"u256 tostr {n} {base}", f"u256 toString #{t}", to_base(x, base))
        add(f"u256 parse {x} 0", f"u256 string constructor #{t}", x)
        yield from cases; cases.clear()
    yield from cases; cases.clear()
    for t in range(count // 4):
        bound = rand_bits(rng, rng.randrange(1, 600))
        sd = rng.randrange(1, 10 ** 9)

        def check_sample(line: str, bound: int = bound, sd: int = sd) -> None:
            v, bl = map(int, line.split())
            if not 0 <= v < bound or bl != sd % 500 + 1:
                raise Failure(f"sample out of range: {line}")
        cases.append((f"sample {bound} {sd}", f"sample #{t}", check_sample))
        d = rng.uniform(-1e300, 1e300) if t % 3 else rng.uniform(-1e18, 1e18)
        add(f"fromdouble {d!r}", f"fromDouble #{t}", int(d))
        yield from cases; cases.clear()
    yield from cases; cases.clear()
    for t, limbs in enumerate([1, 2, 3, 8, 33, 64, 65, 257, 300, 512, 1024, 2048] if mode != "quick" else [1, 2, 3, 8, 33, 64, 65]):
        a = rand_bits(rng, 32 * limbs - rng.randrange(0, 31))
        encoded = format(a, "X")
        shift = (-a.bit_length()) % 32
        normalized = a << shift
        add(f"newton {encoded}", f"newtonRaphsonInv #{t} ({limbs} limbs)", format((1 << (64 * limbs)) // normalized, "X"))
    yield from cases; cases.clear()
    for t in range(count // 3):
        n = rng.choice([1, 2, 5, 23, 24, 25, 40, 60, 100, 160, 700, 1300, 2500] if mode != "quick" else [1, 2, 5, 23, 24, 25, 40, 60])
        m = rng.choice([1, 2, max(1, n // 2), n, n + 1, 2 * n + 1])
        if m > n:
            n, m = m, n
        a = rand_bits(rng, 64 * n - rng.randrange(0, 63)) if t % 4 else (1 << (64 * n)) - 1
        b = rand_bits(rng, 64 * m - rng.randrange(0, 63)) if t % 4 else (1 << (64 * m)) - 1
        for kind in ("basecase", "dispatch", "transform", "ssa"):
            add(f"mulk {kind} {a} {b}", f"mul backend {kind} #{t}", a * b)
        if m > n // 2:
            add(f"mulk karatsuba {a} {b}", f"mul backend karatsuba #{t}", a * b)
        if n >= 2 * m:
            add(f"mulk unbalanced {a} {b}", f"mul backend unbalanced #{t}", a * b)
        if n == m:
            add(f"mulk square {a} {a}", f"mul backend square #{t}", a * a)
            add(f"mulk karatsubasquare {a} {a}", f"mul backend karatsubaSquare #{t}", a * a)
            add(f"mulk truncated {a} {b}", f"mul backend truncated #{t}", (a * b) % (1 << (64 * n)))
            if n >= 6:
                add(f"mulk toom3 {a} {b}", f"mul backend toom3 #{t}", a * b)
                add(f"mulk toom3square {a} {a}", f"mul backend toom3Square #{t}", a * a)
        if m >= 2:
            q = rand_bits(rng, 64 * rng.choice([1, 2, 5, 40, 41, 100]) - rng.randrange(0, 63))
            r = rng.randrange(0, b)
            aa = q * b + r
            bn = b if (b >> (64 * m - 1)) & 1 else None
            add(f"divk dispatch {aa} {b}", f"div backend dispatch #{t}", f"{q} {r}")
            if bn:
                add(f"divk basecase {aa} {b}", f"div backend basecase #{t}", f"{q} {r}")
                add(f"divk newton {aa} {b}", f"div backend newton #{t}", f"{q} {r}")
                if m >= 3:
                    add(f"divk bz {aa} {b}", f"div backend burnikelZiegler #{t}", f"{q} {r}")
            bo = b | 1
            add(f"divk hensel {q * bo} {bo}", f"div backend hensel #{t}", f"{q} 0")
        g = rand_bits(rng, rng.randrange(1, 64 * m + 1))
        x, y = a * g, b * g
        add(f"gcdk binary {x} {y}", f"gcd backend binary #{t}", math.gcd(x, y))

        def check_half(line: str, x: int = x, y: int = y, kind: str = "") -> None:
            ch, ha, hb, A, B, C, D = map(int, line.split())
            ox, oy = (abs(x), abs(y)) if abs(x) >= abs(y) else (abs(y), abs(x))
            if ch and (A * ox + B * oy != ha or C * ox + D * oy != hb or abs(A * D - B * C) != 1 or ha < hb or hb < 0):
                raise Failure(f"half-GCD matrix invalid: {line}")
            if math.gcd(ha, hb) != math.gcd(x, y):
                raise Failure(f"half-GCD changed the gcd: {line}")
        cases.append((f"gcdk halfgcd {x} {y}", f"gcd backend halfGcd #{t}", check_half))
        cases.append((f"gcdk lehmer {x} {y}", f"gcd backend lehmerHalfGcd #{t}", check_half))
        yield from cases; cases.clear()
    yield from cases; cases.clear()


def check_double(line: str, a: int) -> None:
    try:
        expected = float(a)
    except OverflowError:
        expected = math.inf if a > 0 else -math.inf
    got = float(line)
    if got != expected:
        raise Failure(f"toDouble: expected {expected!r}, got {line!r}")


def run_protocol(executable: str, label: str, seed: int, mode: str, env: dict[str, str], timeout: int) -> int:
    """Streams the generated cases through one C++ protocol process and validates each answer as it arrives; returns the case count."""
    proc = subprocess.Popen([executable, "--protocol"], cwd="/tmp", env=env, text=True, bufsize=1 << 16,
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    expected: queue.Queue = queue.Queue()
    feeder_error: list[BaseException] = []

    def feed() -> None:
        try:
            for command, description, checker in build_cases(seed, mode):
                expected.put((command, description, checker))
                proc.stdin.write(command + "\n")
        except BaseException as error:  # noqa: BLE001 - reported by the main thread
            feeder_error.append(error)
        finally:
            expected.put(None)
            try:
                proc.stdin.close()
            except OSError:
                pass

    feeder = threading.Thread(target=feed, daemon=True)
    feeder.start()
    start = time.monotonic()
    index = 0
    try:
        for line in proc.stdout:
            if time.monotonic() - start > timeout:
                raise Failure(f"{label}: protocol timeout after {timeout}s at case {index + 1}")
            item = expected.get()
            if item is None:
                raise Failure(f"{label}: unexpected extra output line {line[:120]!r}")
            command, description, checker = item
            line = line.rstrip("\n")
            index += 1
            try:
                if callable(checker):
                    checker(line)
                elif line != checker:
                    shown = lambda s: s if len(s) <= 200 else s[:90] + "..." + s[-90:]
                    raise Failure(f"expected {shown(checker)!r}, got {shown(line)!r}")
            except Failure as error:
                shown_command = command if len(command) <= 300 else command[:140] + "..." + command[-140:]
                raise Failure(f"{label} case {index} ({description}), command={shown_command!r}: {error}") from error
        feeder.join(timeout=60)
        stderr = proc.stderr.read()
        code = proc.wait(timeout=60)
        if feeder_error:
            raise Failure(f"{label}: case generation failed: {feeder_error[0]!r}")
        remaining = expected.get() if not expected.empty() else None
        if remaining is not None or code != 0:
            raise Failure(f"{label}: protocol process exit={code} after {index} answers"
                          + (f", unanswered case {remaining[1]}" if remaining else "") + f"\nstderr:\n{stderr[-3000:]}")
    finally:
        if proc.poll() is None:
            proc.kill()
            proc.wait()
    return index


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("quick", "full", "stress"), default=os.environ.get("CP_TEST_MODE", "full"))
    parser.add_argument("--seed", type=int, default=int(os.environ.get("CP_TEST_SEED", "42")))
    parser.add_argument("--rounds", type=int, default=1, help="stress mode: rounds per configuration, seeds seed, seed + 1, ...")
    parser.add_argument("--scalar-only", action="store_true", help="explicitly skip the optional ISA configurations")
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
    scalar = ["-march=x86-64", "-mno-avx", "-mno-avx2", "-mno-fma", "-mno-bmi2"]
    vector = ["-march=x86-64", "-mavx2", "-mfma", "-mbmi2"]
    variants = [("optimized-scalar", ["-O2", "-DNDEBUG", *scalar]), ("checked-scalar", ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", *scalar])]
    cpu = Path("/proc/cpuinfo")
    cpu_flags = set(cpu.read_text().split()) if cpu.exists() else set()
    accelerated = not args.scalar_only and {"avx2", "fma", "bmi2"} <= cpu_flags
    if accelerated:
        variants.append(("optimized-avx2", ["-O2", "-DNDEBUG", *vector]))
    else:
        report("SKIP", "AVX2/FMA/BMI2 execution: " + ("requested --scalar-only" if args.scalar_only else "CPU lacks AVX2/FMA/BMI2"))
    if args.mode != "quick":
        sanitizer = ["-O1", "-g", "-D_GLIBCXX_ASSERTIONS", "-DINFINT_SCRATCH_HEAP", "-fno-omit-frame-pointer", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie"]
        variants.append(("sanitized-scalar", [*sanitizer, *scalar]))
        if accelerated:
            variants.append(("sanitized-avx2", [*sanitizer, *vector]))
    report("RUN", f"InfInt mode={args.mode} seed={args.seed}; schoolbook/identity/bit-model/sieve oracles and the Python big-integer protocol")
    start = time.monotonic()
    try:
        with tempfile.TemporaryDirectory(prefix="cp-infint-tests-") as directory:
            alone = Path(directory) / "alone.cpp"
            alone.write_text(f'#include "{ROOT / "01-Core/07-infint.hpp"}"\nint main() {{ iint a("12345678901234567890"); return (a * a) % 7 == InfInt(2) ? 0 : 1; }}\n')
            execute([*compiler, *flags, "-fsyntax-only", str(alone)], "header-alone compile", 180, env)
            aggregate = Path(directory) / "aggregate.cpp"
            aggregate.write_text(f'#include "{ROOT / "01-Core/99-all.hpp"}"\n#include "{ROOT / "01-Core/07-infint.hpp"}"\nint main() {{ return 0; }}\n')
            execute([*compiler, "-std=gnu++20", "-fsyntax-only", str(aggregate)], "99-all aggregate compile (unverified siblings are not warning-clean)", 180, env)
            report("PASS", "header-alone and 99-all aggregate compilation")
            for name, options in variants:
                executable = str(Path(directory) / name)
                label = f"{name} mode={args.mode} seed={args.seed}"
                progress(f"compile {label}")
                execute([*compiler, *flags, *options, str(SOURCE), str(SECOND), "-o", executable], f"compile {label}", 400, env)
                report("PASS", f"{name} compilation (two translation units)")
                run_mode = "full" if args.mode == "stress" and name.startswith("sanitized") else args.mode
                for round_index in range(rounds):
                    seed = (args.seed + round_index) % 2**64
                    label = f"{name} mode={run_mode} seed={seed}"
                    progress(label)
                    limit = 3600 if run_mode == "stress" or name.startswith("sanitized") else 1800
                    output = execute([executable, "--mode", run_mode, "--seed", str(seed)], label, limit, env)
                    if sys.stdout.isatty():
                        print("\r\033[K", end="")
                    for line in output.splitlines():
                        print(f"  [{name}] {line}", flush=True)
                    report("PASS", label)
                    progress(f"{label} protocol")
                    cases = run_protocol(executable, label, seed, run_mode, env, limit)
                    report("PASS", f"{label} protocol ({cases} Python-oracle cases)")
                if name == "checked-scalar":
                    for case in INVALID_CASES:
                        progress(f"precondition {case}")
                        execute([executable, "--invalid", case], f"precondition {case} seed={args.seed}", 60, env, True)
                    report("PASS", f"checked-scalar preconditions ({len(INVALID_CASES)} cases)")
    except (Failure, OSError) as error:
        report("FAIL", f"InfInt mode={args.mode} seed={args.seed}: {error}")
        return 1
    report("PASS", f"InfInt {len(variants)} configurations in {time.monotonic() - start:.2f}s")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
