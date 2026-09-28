from __future__ import annotations

import os
from pathlib import Path
import random
import subprocess
import sys
import threading
import time

GREEN = "\033[1;32m"
RED = "\033[1;31m"
RESET = "\033[0m"
ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path(__file__).with_suffix(".cpp")
if hasattr(sys, "set_int_max_str_digits"):
    sys.set_int_max_str_digits(0)


class Progress:
    def __init__(self, label: str, total: int) -> None:
        self.label, self.total = label, total
        self.tty = sys.stderr.isatty()
        self.last, self.next = -1, 0
        self.update(0)

    def update(self, done: int) -> None:
        pct = 100 * done // self.total if self.total else 100
        if self.tty:
            if pct == self.last:
                return
            self.last = pct
            fill = 32 * pct // 100
            print(f"\r    [{self.label}] [{'#' * fill}{'-' * (32 - fill)}] "
                  f"{pct:3d}% ({done}/{self.total})", end="", file=sys.stderr, flush=True)
            if done == self.total:
                print(file=sys.stderr)
        elif pct >= self.next or done == self.total:
            print(f"    [{self.label}] {pct:3d}% ({done}/{self.total})",
                  file=sys.stderr, flush=True)
            self.next = min(100, 10 * (pct // 10 + 1))


def begin_stage(index: int, total: int, name: str) -> float:
    print(f"[{index}/{total}] {name} ...", file=sys.stderr, flush=True)
    return time.perf_counter()


def finish_stage(start: float, detail: str = "") -> None:
    suffix = f", {detail}" if detail else ""
    print(f"    {GREEN}PASS{RESET} ({time.perf_counter() - start:.2f}s{suffix})",
          file=sys.stderr, flush=True)


def run_capture(command: list[str], timeout: int, input_text: str | None = None) -> subprocess.CompletedProcess[str]:
    env = os.environ.copy()
    env["INFINT_TEST_PROGRESS"] = "tty" if sys.stderr.isatty() else "plain"
    process = subprocess.Popen(command, cwd=ROOT, stdin=subprocess.PIPE if input_text is not None else subprocess.DEVNULL,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env)
    assert process.stderr is not None
    stderr_encoding = process.stderr.encoding or "utf-8"
    stdout_parts: list[str] = []
    stderr_parts: list[bytes] = []

    def read_stdout() -> None:
        assert process.stdout is not None
        while chunk := process.stdout.read(65536):
            stdout_parts.append(chunk)

    def read_stderr() -> None:
        assert process.stderr is not None
        source, sink = process.stderr.buffer, getattr(sys.stderr, "buffer", None)
        while char := source.read(1):
            stderr_parts.append(char)
            if sink is None:
                sys.stderr.write(char.decode(stderr_encoding, errors="replace"))
            else:
                sink.write(char)
            if char == b"\n" or char == b"\r":
                (sys.stderr if sink is None else sink).flush()

    def write_stdin() -> None:
        assert process.stdin is not None
        try:
            process.stdin.write(input_text or "")
        except BrokenPipeError:
            pass
        finally:
            process.stdin.close()

    threads = [threading.Thread(target=read_stdout), threading.Thread(target=read_stderr)]
    if input_text is not None:
        threads.append(threading.Thread(target=write_stdin))
    for thread in threads:
        thread.start()
    try:
        process.wait(timeout=timeout)
    except subprocess.TimeoutExpired as exc:
        process.kill(); process.wait()
        for thread in threads:
            thread.join()
        exc.output = "".join(stdout_parts)
        exc.stderr = b"".join(stderr_parts).decode(stderr_encoding, errors="replace")
        raise
    for thread in threads:
        thread.join()
    sys.stderr.flush()
    stderr = b"".join(stderr_parts).decode(stderr_encoding, errors="replace")
    return subprocess.CompletedProcess(command, process.returncode, "".join(stdout_parts), stderr)


def trunc_divmod(a: int, b: int) -> tuple[int, int]:
    q = abs(a) // abs(b)
    if (a < 0) != (b < 0):
        q = -q
    return q, a - q * b


def format_base(a: int, base: int) -> str:
    chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    neg, a = a < 0, abs(a)
    if a == 0:
        return "0"
    res: list[str] = []
    while a:
        a, d = divmod(a, base)
        res.append(chars[d])
    return ("-" if neg else "") + "".join(reversed(res))


def rand_bits(rng: random.Random, bits: int, signed: bool = True) -> int:
    if bits <= 0:
        return 0
    x = (1 << (bits - 1)) | rng.getrandbits(bits - 1)
    return -x if signed and rng.randrange(2) else x


def rand_decimal(rng: random.Random, digits: int, signed: bool = True) -> int:
    s = str(rng.randrange(1, 10)) + "".join(str(rng.randrange(10)) for _ in range(digits - 1))
    x = int(s)
    return -x if signed and rng.randrange(2) else x


def shown(s: str) -> str:
    return s if len(s) <= 240 else s[:110] + "..." + s[-110:]


def main() -> int:
    stages = 6
    stage_start = begin_stage(1, stages, "Building deterministic differential corpus "
                                      "(seed=0xBB67AE8584CAA73B)")
    rng = random.Random(0xBB67AE8584CAA73B)
    cases: list[tuple[str, str, str]] = []

    def add(command: str, expected: str | int, description: str) -> None:
        cases.append((command, str(expected), description))

    boundaries = [
        -(1 << 127), -(1 << 64), -(1 << 63), -(10**20), -1, 0, 1,
        10**20, (1 << 63) - 1, (1 << 64) - 1, (1 << 127) - 1,
    ]
    for a in boundaries:
        for b in boundaries:
            add(f"add {a} {b}", a + b, f"boundary add {a},{b}")
            add(f"sub {a} {b}", a - b, f"boundary sub {a},{b}")
            add(f"mul {a} {b}", a * b, f"boundary mul {a},{b}")
            add(f"cmp {a} {b}", -1 if a < b else 1 if a > b else 0, f"boundary cmp {a},{b}")
            if b:
                q, r = trunc_divmod(a, b)
                add(f"div {a} {b}", f"{q} {r}", f"boundary div {a},{b}")

    for k in range(1, 13):
        p = 100_000**k
        values = [-(p + 1), -p, -(p - 1), p - 1, p, p + 1]
        for ai, a in enumerate(values):
            for bi, b in enumerate(values):
                add(f"add {a} {b}", a + b, f"base-limb carry add k={k}, pair={ai}/{bi}")
                add(f"sub {a} {b}", a - b, f"base-limb borrow sub k={k}, pair={ai}/{bi}")
                add(f"mul {a} {b}", a * b, f"base-limb product k={k}, pair={ai}/{bi}")
                add(f"cmp {a} {b}", -1 if a < b else 1 if a > b else 0,
                    f"base-limb compare k={k}, pair={ai}/{bi}")

    limb_boundaries = sorted({100_000**k + d for k in range(1, 8) for d in [-2, -1, 0, 1, 2]})
    for ai, a in enumerate(limb_boundaries):
        for bi, b in enumerate(limb_boundaries):
            q, r = trunc_divmod(a, b)
            add(f"div {a} {b}", f"{q} {r}", f"limb-boundary div {ai},{bi}")
    for ci, (a, b) in enumerate([(759028842393192494218867898051, 619397960225983),
                                  (500005000000000, 5000099999)]):
        for si, (sa, sb) in enumerate([(1, 1), (-1, 1), (1, -1), (-1, -1)]):
            x, y = sa * a, sb * b; q, r = trunc_divmod(x, y)
            add(f"div {x} {y}", f"{q} {r}", f"clamped Knuth estimate regression {ci}/{si}")
    for tc in range(2000):
        al, bl = rng.randrange(2, 15), rng.randrange(2, 8)
        a = sum(rng.randrange(100_000) * 100_000**i for i in range(al - 1))
        a += rng.randrange(1, 100_000) * 100_000 ** (al - 1)
        b = sum(rng.randrange(100_000) * 100_000**i for i in range(bl - 1))
        b += rng.randrange(1, 100_000) * 100_000 ** (bl - 1)
        q, r = trunc_divmod(a, b)
        add(f"div {a} {b}", f"{q} {r}", f"focused Knuth fuzz #{tc}")

    bit_choices = [1, 16, 31, 32, 33, 63, 64, 65, 127, 256, 511, 1024, 4096, 10_000]
    for tc in range(450):
        a = rand_bits(rng, rng.choice(bit_choices))
        b = rand_bits(rng, rng.choice(bit_choices))
        add(f"add {a} {b}", a + b, f"random add #{tc}")
        add(f"sub {a} {b}", a - b, f"random sub #{tc}")
        add(f"mul {a} {b}", a * b, f"random mul #{tc}")
        add(f"cmp {a} {b}", -1 if a < b else 1 if a > b else 0, f"random cmp #{tc}")
        add(f"alias {a}", a * a, f"random alias square #{tc}")
        if b:
            q, r = trunc_divmod(a, b)
            add(f"div {a} {b}", f"{q} {r}", f"random div #{tc}")

    scalar_choices = [-(1 << 63), -4_294_967_297, -100_001, -1, 1, 100_001,
                      4_294_967_297, ((1 << 64) - 1) // 100_000,
                      ((1 << 64) - 1) // 100_000 + 1, (1 << 63) - 1]
    for a in boundaries:
        add(f"smul {a} 0", 0, f"scalar multiply by zero {a}")
    for b in scalar_choices:
        add(f"sdiv 0 {b}", 0, f"zero scalar quotient {b}")
        add(f"smod 0 {b}", 0, f"zero scalar remainder {b}")
    for tc in range(300):
        a = rand_bits(rng, rng.choice([1, 64, 511, 4096, 10_000]))
        b = rng.choice(scalar_choices)
        q, r = trunc_divmod(a, b)
        add(f"smul {a} {b}", a * b, f"scalar mul #{tc}")
        add(f"sdiv {a} {b}", q, f"scalar div #{tc}")
        add(f"smod {a} {b}", r, f"scalar mod #{tc}")

    for digits in [1000, 1100, 1275, 1280, 1285, 2000, 4000, 10_000, 20_000, 100_000]:
        for tc in range(1 if digits == 100_000 else 3):
            a = rand_decimal(rng, digits)
            b = rand_decimal(rng, digits + (tc & 1))
            add(f"mul {a} {b}", a * b, f"NTT product {digits} digits #{tc}")
            add(f"alias {a}", a * a, f"NTT alias square {digits} digits #{tc}")
    for digits in [320, 640, 1280, 2560]:
        a = int("9" * digits)
        add(f"mul {a} {a}", a * a, f"all-nine square {digits} digits")

    division_shapes = [(1, 1), (64, 64), (257, 1), (257, 256), (512, 511),
                       (1024, 64), (2048, 2048), (5000, 2500), (20_000, 10_000)]
    for si, (bbits, qbits) in enumerate(division_shapes):
        b = rand_bits(rng, bbits, False)
        q0 = rand_bits(rng, qbits, False)
        for ri, r0 in enumerate([0, 1, b // 2, b - 1]):
            positive = b * q0 + r0
            for sj, (a, d) in enumerate([(positive, b), (-positive, b), (positive, -b), (-positive, -b)]):
                q, r = trunc_divmod(a, d)
                add(f"div {a} {d}", f"{q} {r}", f"division shape {si}/{ri}/{sj}")
    for tc in range(100):
        a = rand_bits(rng, rng.randrange(1, 20_001))
        b = rand_bits(rng, rng.randrange(1, 10_001))
        q, r = trunc_divmod(a, b)
        add(f"div {a} {b}", f"{q} {r}", f"large random division #{tc}")
    a = rand_decimal(rng, 20_000); b = rand_decimal(rng, 10_000)
    q, r = trunc_divmod(a, b)
    add(f"div {a} {b}", f"{q} {r}", "20k-by-10k-digit division")

    for tc in range(80):
        a = rand_bits(rng, rng.randrange(1, 1025))
        for base in range(2, 37):
            encoded = format_base(a, base)
            add(f"tobase {a} {base}", encoded, f"to base {base} #{tc}")
            add(f"frombase {base} {encoded}", a, f"from base {base} #{tc}")

    finish_stage(stage_start, f"{len(cases)} cases")
    cxx = os.environ.get("CXX", "g++")
    exe = SOURCE.with_name(f"infintmini_test_{os.getpid()}.exe")
    debug_exe = SOURCE.with_name(f"infintmini_debug_{os.getpid()}.exe")
    common = [cxx, "-std=c++20", "-mno-avx2", "-Wno-sign-compare", str(SOURCE)]
    try:
        stage_start = begin_stage(2, stages, "Building optimized Mini tester")
        subprocess.run(common[:2] + ["-O2", "-Wall", "-Wextra", "-Wpedantic", "-Wduplicated-cond",
                       "-Wlogical-op"] + common[2:] + ["-o", str(exe)], cwd=ROOT, check=True, timeout=180)
        finish_stage(stage_start)

        stage_start = begin_stage(3, stages, "Running optimized C++ self-test phases")
        self_result = run_capture([str(exe)], timeout=180)
        if self_result.returncode != 0:
            raise RuntimeError(f"C++ self-test exited with code {self_result.returncode}: "
                               f"{self_result.stdout!r} {self_result.stderr!r}")
        if "PASS" not in self_result.stdout:
            raise RuntimeError(f"C++ self-test did not pass: {self_result.stdout!r} {self_result.stderr!r}")
        finish_stage(stage_start)

        stage_start = begin_stage(4, stages, "Running and verifying Python differential corpus")
        payload = str(len(cases)) + "\n" + "\n".join(case[0] for case in cases) + "\n"
        result = run_capture([str(exe), "--protocol"], timeout=300, input_text=payload)
        if result.returncode != 0:
            raise RuntimeError(f"protocol process exited with code {result.returncode}: "
                               f"{result.stdout!r} {result.stderr!r}")
        lines = result.stdout.splitlines()
        if len(lines) != len(cases):
            raise RuntimeError(f"expected {len(cases)} output lines, got {len(lines)}; stderr={result.stderr!r}")
        progress = Progress("Python verification", len(cases))
        for index, (actual, (command, expected, description)) in enumerate(zip(lines, cases), 1):
            if actual != expected:
                raise RuntimeError(f"case {index} ({description}, command={shown(command)!r}): "
                                   f"expected {shown(expected)!r}, got {shown(actual)!r}")
            progress.update(index)
        finish_stage(stage_start, f"{len(cases)} cases")

        stage_start = begin_stage(5, stages, "Building debug-iterator Mini tester")
        subprocess.run(common[:2] + ["-O1", "-D_GLIBCXX_DEBUG"] + common[2:] + ["-o", str(debug_exe)],
                       cwd=ROOT, check=True, timeout=180)
        finish_stage(stage_start)

        stage_start = begin_stage(6, stages, "Running debug-iterator C++ self-test phases")
        debug_result = run_capture([str(debug_exe)], timeout=180)
        if debug_result.returncode != 0:
            raise RuntimeError(f"debug C++ self-test exited with code {debug_result.returncode}: "
                               f"{debug_result.stdout!r} {debug_result.stderr!r}")
        if "PASS" not in debug_result.stdout:
            raise RuntimeError(f"debug C++ self-test did not pass: "
                               f"{debug_result.stdout!r} {debug_result.stderr!r}")
        finish_stage(stage_start)
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired, RuntimeError) as exc:
        print(f"{RED}FAIL: 1-Core_06-infintmini_tester.py: {exc}{RESET}", file=sys.stderr)
        return 1
    finally:
        exe.unlink(missing_ok=True)
        debug_exe.unlink(missing_ok=True)

    print(f"{GREEN}PASS: 1-Core_06-infintmini_tester.py ({len(cases)} differential cases){RESET}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
