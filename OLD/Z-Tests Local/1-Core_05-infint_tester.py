from __future__ import annotations

import argparse
import math
import os
from pathlib import Path
import random
import subprocess
import sys
import time
from typing import Callable

GREEN = "\033[1;32m"
RED = "\033[1;31m"
RESET = "\033[0m"
ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path(__file__).with_suffix(".cpp")
if hasattr(sys, "set_int_max_str_digits"):
    sys.set_int_max_str_digits(0)


class TestFailure(RuntimeError):
    pass


def status(scope: str, message: str) -> None:
    print(f"[{scope}] {message}", file=sys.stderr, flush=True)


class Progress:
    def __init__(self, label: str, total: int) -> None:
        self.label, self.total = label, total
        self.tty = sys.stderr.isatty()
        self.last, self.next = -1, 0
        self.update(0)

    def update(self, done: int) -> None:
        percent = 100 if not self.total else 100 * done // self.total
        if self.tty:
            if percent == self.last:
                return
            self.last = percent
            filled = 32 * percent // 100
            print(f"\r    [{self.label}] [{'#' * filled}{'-' * (32 - filled)}] "
                  f"{percent:3d}% ({done}/{self.total})", end="", file=sys.stderr, flush=True)
            if done == self.total:
                print(file=sys.stderr, flush=True)
        elif percent >= self.next or done == self.total:
            print(f"    [{self.label}] {percent:3d}% ({done}/{self.total})",
                  file=sys.stderr, flush=True)
            self.next = min(100, 10 * (percent // 10 + 1))


def trunc_divmod(a: int, b: int) -> tuple[int, int]:
    q = abs(a) // abs(b)
    if (a < 0) != (b < 0):
        q = -q
    return q, a - q * b


def format_base(a: int, base: int, compact: bool = False) -> str:
    chars = "0123456789ABCDEF"
    neg = a < 0
    a = abs(a)
    if a == 0:
        return "0"
    digs: list[int] = []
    while a:
        a, d = divmod(a, base)
        digs.append(d)
    digs.reverse()
    sign = "-" if neg else ""
    if base <= 16:
        return sign + "".join(chars[d] for d in digs)
    sep = "," if compact else ", "
    return sign + "[" + sep.join(map(str, digs)) + "]"


Check = Callable[[str], None]


def exact(expected: str) -> Check:
    def check(actual: str) -> None:
        if actual != expected:
            shown_expected = expected if len(expected) <= 240 else expected[:110] + "..." + expected[-110:]
            shown_actual = actual if len(actual) <= 240 else actual[:110] + "..." + actual[-110:]
            raise TestFailure(f"expected {shown_expected!r}, got {shown_actual!r}")

    return check


def rand_exact_bits(rng: random.Random, bits: int, signed: bool = False) -> int:
    if bits <= 0:
        return 0
    x = (1 << (bits - 1)) | rng.getrandbits(bits - 1)
    return -x if signed and rng.randrange(2) else x


def build_cases(seed: int = 0x13198A2E03707344) -> list[tuple[str, str, Check]]:
    rng = random.Random(seed)
    cases: list[tuple[str, str, Check]] = []

    def add(command: str, description: str, expected: str | int) -> None:
        cases.append((command, description, exact(str(expected))))

    boundaries = [
        -(1 << 127), -(1 << 64), -(1 << 63), -(1 << 32), -1, 0, 1,
        (1 << 32) - 1, 1 << 32, (1 << 63) - 1, (1 << 64) - 1, (1 << 127) - 1,
    ]
    for a in boundaries:
        add(f"inc {a}", f"increment {a}", a + 1)
        add(f"dec {a}", f"decrement {a}", a - 1)
        add(f"not {a}", f"bitwise not {a}", ~a)
    for limbs in [1, 3, 4, 5, 7, 8, 9, 31, 32, 33, 257]:
        a = rand_exact_bits(rng, 32 * limbs, True)
        for name, base in [("bin", 2), ("oct", 8), ("hex", 16)]:
            add(f"{name} {a}", f"direct {name} formatting ({limbs} limbs)", format_base(a, base))
    for t in range(350):
        abits = rng.choice([1, 31, 32, 33, 63, 64, 65, 127, 128, 129, 511, 1024, 4096, 8192])
        bbits = rng.choice([1, 31, 32, 33, 63, 64, 65, 127, 128, 129, 511, 1024, 4096])
        a = rand_exact_bits(rng, abits, True)
        b = rand_exact_bits(rng, bbits, True)
        add(f"add {a} {b}", f"random add #{t}", a + b)
        add(f"sub {a} {b}", f"random subtract #{t}", a - b)
        add(f"and {a} {b}", f"random and #{t}", a & b)
        add(f"or {a} {b}", f"random or #{t}", a | b)
        add(f"xor {a} {b}", f"random xor #{t}", a ^ b)
        add(f"cmp {a} {b}", f"random compare #{t}", -1 if a < b else 1 if a > b else 0)
        shf = rng.randrange(0, 10_000)
        add(f"shl {a} {shf}", f"random left shift #{t}", a << shf)
        add(f"shr {a} {shf}", f"random right shift #{t}", a >> shf)
        add(f"ctz {a}", f"random ctz #{t}", 0 if a == 0 else (abs(a) & -abs(a)).bit_length() - 1)
        add(f"popcount {a}", f"random popcount #{t}", abs(a).bit_count())

    mul_shapes = [
        (1, 1), (31, 33), (32 * 63, 32 * 70), (32 * 64, 32 * 255),
        (32 * 64, 32 * 256), (32 * 65, 32 * 65), (32 * 112, 32 * 112),
        (32 * 127, 32 * 127), (32 * 129, 32 * 129), (32 * 192, 32 * 192),
        (32 * 224, 32 * 224), (32 * 255, 32 * 255), (32 * 256, 32 * 256),
        (32 * 257, 32 * 257), (32 * 320, 32 * 320), (32 * 384, 32 * 384),
        (32 * 1023, 32 * 1023), (32 * 1024, 32 * 1024), (32 * 64, 32 * 4096),
        (32 * 128, 32 * 4096), (32 * 512, 32 * 4096), (100_000, 100_000),
    ]
    for t, (abits, bbits) in enumerate(mul_shapes):
        a = rand_exact_bits(rng, abits, True)
        b = rand_exact_bits(rng, bbits, True)
        add(f"mul {a} {b}", f"threshold multiplication #{t} ({abits}x{bbits} bits)", a * b)
        if abits == bbits:
            add(f"mul {a} {a}", f"threshold square #{t} ({abits} bits)", a * a)
    for limbs in [64, 257, 1024]:
        a = (1 << (32 * limbs)) - 1
        add(f"mul {a} {a}", f"all-ones multiplication ({limbs} limbs)", a * a)

    division_shapes = [(1, 1), (32, 64), (255, 100), (256, 256), (257, 1), (257, 256),
                       (300, 301), (512, 511), (513, 512), (1024, 64), (300, 1500)]
    for t, (b_limbs, q_limbs) in enumerate(division_shapes):
        b = rand_exact_bits(rng, 32 * b_limbs)
        q = rand_exact_bits(rng, 32 * q_limbs)
        for ri, r in enumerate([0, 1, b // 2, b - 1]):
            positive = b * q + r
            for si, (a, divisor) in enumerate([(positive, b), (-positive, b), (positive, -b), (-positive, -b)]):
                expected_q, expected_r = trunc_divmod(a, divisor)
                add(f"div {a} {divisor}", f"division shape #{t} remainder #{ri} sign #{si}", f"{expected_q} {expected_r}")
    for t in range(80):
        b = rand_exact_bits(rng, rng.randrange(1, 18_000))
        a = rand_exact_bits(rng, rng.randrange(1, 35_000), True)
        if rng.randrange(2):
            b = -b
        q, r = trunc_divmod(a, b)
        add(f"div {a} {b}", f"random division #{t}", f"{q} {r}")

    for t in range(60):
        a = rand_exact_bits(rng, rng.randrange(1, 20_000), True)
        b = rand_exact_bits(rng, rng.randrange(1, 20_000), True)
        g = math.gcd(a, b)
        add(f"gcd {a} {b}", f"random gcd #{t}", g)
        add(f"lcm {a} {b}", f"random lcm #{t}", 0 if not a or not b else abs(a // g * b))

        def check_exgcd(line: str, a: int = a, b: int = b, g: int = g) -> None:
            fields = line.split()
            if len(fields) != 3:
                raise TestFailure(f"expected three exGcd fields, got {line!r}")
            actual_g, x, y = map(int, fields)
            if actual_g != g or a * x + b * y != g:
                raise TestFailure("invalid extended-GCD result")

        cases.append((f"exgcd {a} {b}", f"random exGcd #{t}", check_exgcd))

    for a in range(-40, 41):
        add(f"inv {a} 1", f"inverse {a} mod 1", 0)
        add(f"inv {a} -1", f"inverse {a} mod -1", 0)
        for m in range(2, 80):
            expected_inv = -1 if math.gcd(a, m) != 1 else pow(a, -1, m)
            add(f"inv {a} {m}", f"inverse {a} mod {m}", expected_inv)
            add(f"inv {a} {-m}", f"inverse {a} mod {-m}", expected_inv)
    for t in range(150):
        m = rand_exact_bits(rng, rng.randrange(33, 65))
        a = rand_exact_bits(rng, rng.randrange(1, 257), True)
        expected_inv = -1 if math.gcd(a, m) != 1 else pow(a, -1, m)
        add(f"inv {a} {m}", f"64-bit inverse #{t}", expected_inv)
        e = rng.getrandbits(rng.randrange(2, 513))
        add(f"powmod {a} {e} {m}", f"64-bit-modulus powMod #{t}", pow(a, e, m))
        if expected_inv != -1 and t < 50:
            add(f"powmod {a} {-e} {m}", f"64-bit-modulus negative powMod #{t}", pow(a, -e, m))
    for t in range(80):
        m = rand_exact_bits(rng, rng.randrange(2, 14_000))
        a = rand_exact_bits(rng, rng.randrange(1, 14_000), True)
        e = rng.randrange(0, 20_000)
        add(f"powmod {a} {e} {m}", f"random powMod #{t}", pow(a, e, m))
    window_mod = (1 << 127) - 1
    for ebits in [31, 32, 33, 255, 256, 257, 1023, 1024, 1025, 4096, 8192]:
        dense = rand_exact_bits(rng, ebits)
        for shape, e in [("random", dense), ("ones", (1 << ebits) - 1),
                         ("sparse", (1 << (ebits - 1)) | 1)]:
            add(f"powmod 5 {e} {window_mod}", f"windowed powMod {shape} exponent ({ebits} bits)",
                pow(5, e, window_mod))
    negative_e = -rand_exact_bits(rng, 1025)
    add(f"powmod 5 {negative_e} {window_mod}", "windowed powMod negative exponent (1025 bits)",
        pow(5, negative_e, window_mod))
    for a, e, m in [(3, -1, 7), (3, -2, 7), (-3, -5, 11), (2, -1, 4), (0, 0, 17), (5, 0, 1)]:
        try:
            expected = pow(a, e, abs(m))
        except ValueError:
            expected = -1
        add(f"powmod {a} {e} {m}", f"signed powMod {a}^{e} mod {m}", expected)
    for t in range(100):
        a = rand_exact_bits(rng, rng.randrange(1, 35_000))
        add(f"sqrt {a}", f"random sqrt #{t}", math.isqrt(a))
    for a in range(-12, 13):
        for e in range(0, 40):
            add(f"pow {a} {e}", f"small power {a}^{e}", a**e)

    for t in range(80):
        a = rand_exact_bits(rng, rng.randrange(1, 12_000), True)
        for base in [2, 3, 7, 8, 10, 16, 17, 36, 255, 65_521, 1_000_000_007, (1 << 32) - 1]:
            encoded = format_base(a, base)
            add(f"tobase {base} {a}", f"to base {base} #{t}", encoded)
            compact = format_base(a, base, True)
            add(f"frombase {base} {compact}", f"from base {base} #{t}", a)

    for t, limbs in enumerate([257, 300, 329, 511, 512, 513, 1024, 2048, 4096]):
        a = rand_exact_bits(rng, limbs * 32 - rng.randrange(0, 31))
        encoded = format(a, "X")
        shift = (-a.bit_length()) % 32
        normalized = a << shift
        exact_reciprocal = (1 << (64 * limbs)) // normalized

        def check_newton(line: str, exact_reciprocal: int = exact_reciprocal) -> None:
            got = int(line, 16)
            if abs(got - exact_reciprocal) > 4:
                raise TestFailure(f"reciprocal error is {got - exact_reciprocal}, expected an error within 4")

        cases.append((f"newton {encoded}", f"Newton reciprocal #{t} ({limbs} limbs)", check_newton))

    for t, limbs in enumerate([65, 129, 300, 513, 1024, 2048, 4096]):
        a = rand_exact_bits(rng, limbs * 32)
        b = rand_exact_bits(rng, (limbs - rng.randrange(0, max(1, limbs // 8))) * 32)
        if b > a:
            a, b = b, a
        target = (((a.bit_length() + 31) // 32) + 1) // 2

        def check_halfgcd(line: str, a: int = a, b: int = b, target: int = target) -> None:
            fields = line.split()
            if len(fields) != 4:
                raise TestFailure(f"expected four half-GCD fields, got {line!r}")
            A, B, C, D = (int(x, 16) if not x.startswith("-") else -int(x[1:], 16) for x in fields)
            if abs(A * D - B * C) != 1:
                raise TestFailure("half-GCD matrix is not unimodular")
            ta, tb = A * a + B * b, C * a + D * b
            if not (ta >= tb >= 0):
                raise TestFailure("half-GCD matrix does not preserve the expected Euclidean ordering")
            tb_limbs = max(1, (tb.bit_length() + 31) // 32)
            if tb and tb_limbs > target:
                raise TestFailure(f"half-GCD stopped at {tb_limbs} limbs; target is {target}")

        cases.append((f"halfgcd {a:X} {b:X}", f"half-GCD #{t} ({limbs} limbs)", check_halfgcd))
    return cases


def run_variant(executable: Path, variant: str, cases: list[tuple[str, str, Check]]) -> None:
    variant_start = time.perf_counter()
    env = os.environ.copy()
    env["INFINT_TEST_PROGRESS"] = "tty" if sys.stderr.isatty() else "plain"
    env["INFINT_TEST_VARIANT"] = variant
    status(variant, "[2/4] Running named C++ self-test phases")
    start = time.perf_counter()
    self_test = subprocess.run([str(executable)], cwd=ROOT, text=True, stdout=subprocess.PIPE,
                               stderr=None, timeout=240, env=env)
    if self_test.returncode != 0:
        raise TestFailure(f"{variant} C++ self-test failed with code {self_test.returncode}; "
                          f"stderr diagnostics are above.\n{self_test.stdout}")
    status(variant, f"[2/4] C++ self-test {GREEN}PASS{RESET} ({time.perf_counter() - start:.2f}s)")
    payload = str(len(cases)) + "\n" + "\n".join(command for command, _, _ in cases) + "\n"
    status(variant, f"[3/4] Running differential protocol ({len(cases)} cases)")
    start = time.perf_counter()
    run = subprocess.run([str(executable), "--protocol"], cwd=ROOT, input=payload, text=True,
                         stdout=subprocess.PIPE, stderr=None, timeout=300, env=env)
    if run.returncode != 0:
        raise TestFailure(f"{variant} protocol process failed with code {run.returncode}; "
                          "stderr diagnostics are above")
    status(variant, f"[3/4] Differential protocol {GREEN}PASS{RESET} ({time.perf_counter() - start:.2f}s)")
    lines = run.stdout.splitlines()
    if len(lines) != len(cases):
        raise TestFailure(f"{variant} returned {len(lines)} lines for {len(cases)} cases")
    status(variant, f"[4/4] Validating {len(cases)} results against Python")
    start = time.perf_counter()
    progress = Progress(f"{variant} validation", len(cases))
    for index, ((command, description, checker), line) in enumerate(zip(cases, lines)):
        try:
            checker(line)
        except TestFailure as error:
            raise TestFailure(f"{variant} case {index + 1}/{len(cases)} ({description}), "
                              f"command={command!r}: {error}") from error
        progress.update(index + 1)
    status(variant, f"[4/4] Python validation {GREEN}PASS{RESET} ({time.perf_counter() - start:.2f}s)")
    status(variant, f"Variant {GREEN}PASS{RESET} ({time.perf_counter() - variant_start:.2f}s total)")


def main() -> None:
    parser = argparse.ArgumentParser(description="Compile and differentially test 1-Core/05-infint.hpp.")
    parser.add_argument("--scalar-only", action="store_true", help="skip the AVX2 build")
    parser.add_argument("--avx2-only", action="store_true", help="skip the scalar build")
    parser.add_argument("--compiler", default="g++")
    args = parser.parse_args()
    if args.scalar_only and args.avx2_only:
        raise TestFailure("--scalar-only and --avx2-only are mutually exclusive")
    status("setup", "Generating deterministic exhaustive/differential cases (seed=0x13198A2E03707344)")
    start = time.perf_counter()
    cases = build_cases()
    status("setup", f"Generated {len(cases)} cases ({time.perf_counter() - start:.2f}s)")
    variants: list[tuple[str, list[str]]] = []
    if not args.avx2_only:
        variants.append(("scalar", ["-mno-avx2"]))
    if not args.scalar_only:
        variants.append(("AVX2", ["-mavx2"]))
    suffix = ".exe" if os.name == "nt" else ""
    executables: list[Path] = []
    try:
        for variant, target_flags in variants:
            executable = SOURCE.parent / f"infint_test_{variant.lower()}_{os.getpid()}{suffix}"
            executables.append(executable)
            compile_command = [args.compiler, "-std=c++20", "-O2", "-pipe", *target_flags,
                               str(SOURCE), "-o", str(executable)]
            status(variant, f"[1/4] Building with {' '.join(target_flags)}")
            start = time.perf_counter()
            compiled = subprocess.run(compile_command, cwd=ROOT, text=True, capture_output=True, timeout=180)
            if compiled.returncode != 0:
                raise TestFailure(f"{variant} compile failed:\n{compiled.stdout}{compiled.stderr}")
            status(variant, f"[1/4] Build {GREEN}PASS{RESET} ({time.perf_counter() - start:.2f}s)")
            run_variant(executable, variant, cases)
    finally:
        for executable in executables:
            try:
                executable.unlink(missing_ok=True)
            except PermissionError:
                pass
    print(f"{GREEN}PASS: 1-Core_05-infint_tester.py ({', '.join(name for name, _ in variants)}, {len(cases)} cases each){RESET}")


if __name__ == "__main__":
    try:
        main()
    except (TestFailure, subprocess.TimeoutExpired, OSError) as error:
        print(f"{RED}FAIL: 1-Core_05-infint_tester.py: {error}{RESET}", file=sys.stderr)
        raise SystemExit(1)
