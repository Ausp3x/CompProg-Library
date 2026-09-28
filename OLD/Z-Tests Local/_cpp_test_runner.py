import os
import shutil
import subprocess
import sys
import uuid
from pathlib import Path

GREEN, RED, RESET = "\033[1;32m", "\033[1;31m", "\033[0m"
TEST_DIR = Path(__file__).resolve().parent
ROOT = TEST_DIR.parent
COMMON_FLAGS = [
    "-std=gnu++20", "-Wall", "-Wextra", "-Wshadow", "-Wconversion", "-pedantic"
]
VARIANTS = [
    ("optimized", ["-O2", "-pipe", "-DNDEBUG"]),
    ("checked", ["-O0", "-g", "-D_GLIBCXX_ASSERTIONS"]),
]


class TestFailure(RuntimeError):
    pass


def details(result: subprocess.CompletedProcess[str]) -> str:
    parts = []
    if result.stdout:
        parts.append("stdout:\n" + result.stdout.rstrip())
    if result.stderr:
        parts.append("stderr:\n" + result.stderr.rstrip())
    return "\n".join(parts)


def checked_run(command: list[str], label: str, timeout: int) -> None:
    result = subprocess.run(
        command, cwd=ROOT, text=True, capture_output=True, timeout=timeout,
        encoding="utf-8", errors="replace"
    )
    if result.returncode != 0:
        output = details(result)
        raise TestFailure(
            f"{label} exited with code {result.returncode}"
            + (f"\n{output}" if output else "")
        )


def draw_progress(done: int, total: int, label: str) -> None:
    width = 28
    filled = width * done // total
    bar = "#" * filled + "-" * (width - filled)
    text = f"\r[{bar}] {done:>{len(str(total))}}/{total} {label}"
    sys.stdout.write(text[:118].ljust(118))
    sys.stdout.flush()


def run_cpp_suite(
    suite: str, cpp_sources: list[str], python_checks: list[str] | None = None
) -> None:
    python_checks = python_checks or []
    compiler = shutil.which(os.environ.get("CXX", "g++"))
    if compiler is None:
        print(f"{RED}FAIL: {suite}: C++ compiler not found{RESET}", file=sys.stderr)
        raise SystemExit(1)

    total = 2 * len(VARIANTS) * len(cpp_sources) + len(python_checks)
    done = 0
    draw_progress(done, total, "starting")
    suffix = ".exe" if os.name == "nt" else ""
    tmp_dir = TEST_DIR / f".core_test_{os.getpid()}_{uuid.uuid4().hex}"

    try:
        tmp_dir.mkdir()
        for variant, flags in VARIANTS:
            for source_name in cpp_sources:
                source = TEST_DIR / source_name
                executable = tmp_dir / f"{source.stem}_{variant}{suffix}"
                label = f"{variant}: compile {source.name}"
                checked_run(
                    [compiler, *COMMON_FLAGS, *flags, str(source), "-o", str(executable)],
                    label, 120
                )
                done += 1
                draw_progress(done, total, label)

                label = f"{variant}: run {source.name}"
                checked_run([str(executable)], label, 180)
                done += 1
                draw_progress(done, total, label)

        for check_name in python_checks:
            label = f"run {check_name}"
            checked_run([sys.executable, str(TEST_DIR / check_name)], label, 120)
            done += 1
            draw_progress(done, total, label)
    except (TestFailure, subprocess.TimeoutExpired, OSError) as error:
        sys.stdout.write("\n")
        print(f"{RED}FAIL: {suite}: {error}{RESET}", file=sys.stderr)
        raise SystemExit(1)
    finally:
        shutil.rmtree(tmp_dir, ignore_errors=True)

    sys.stdout.write("\n")
    print(f"{GREEN}PASS: {suite}{RESET}")
