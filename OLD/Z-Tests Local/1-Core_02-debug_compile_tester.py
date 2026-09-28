import shutil
import subprocess
from pathlib import Path

GREEN, RED, RESET = "\033[1;32m", "\033[1;31m", "\033[0m"
HEADER = (Path(__file__).resolve().parents[1] / "1-Core" / "02-debug.hpp").as_posix()


def fail(label: str, result: subprocess.CompletedProcess[str]) -> None:
    print(f"{RED}FAIL: 1-Core_02-debug_compile_tester.py: {label}{RESET}")
    if result.stdout:
        print("compiler stdout:\n" + result.stdout.rstrip())
    if result.stderr:
        print("compiler stderr:\n" + result.stderr.rstrip())
    raise SystemExit(1)


def main() -> None:
    compiler = shutil.which("g++")
    if compiler is None:
        print(f"{RED}FAIL: 1-Core_02-debug_compile_tester.py: g++ not found{RESET}")
        raise SystemExit(1)
    source = f'''#define LOCAL
#include "{HEADER}"
int main() {{ trace("first"); trace("second"); }}
'''
    result = subprocess.run(
        [compiler, "-std=gnu++20", "-Wall", "-Wextra", "-pedantic", "-x", "c++", "-fsyntax-only", "-"],
        input=source, capture_output=True, text=True, encoding="utf-8", errors="replace"
    )
    if result.returncode != 0:
        fail("multiple trace() calls on one compressed source line do not compile", result)
    print(f"{GREEN}PASS: 1-Core_02-debug_compile_tester{RESET}")


if __name__ == "__main__":
    main()
