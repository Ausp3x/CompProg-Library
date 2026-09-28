"""Standalone headers, macro syntax, and ODR linkage in LOCAL/mixed builds."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    compiler = shutil.which(os.getenv("CXX", "g++"))
    if compiler is None:
        raise SystemExit("FAIL: CXX compiler not found")
    command = []

    def run(args, label, source=None):
        nonlocal command
        command = [str(x) for x in args]
        result = subprocess.run(command, input=source, text=True, capture_output=True, timeout=120)
        if result.returncode:
            raise RuntimeError(f"{label}: exit={result.returncode}\n{result.stdout}{result.stderr}")
        print(f"PASS {label}")

    try:
        flags = [compiler, "-std=gnu++20", "-O2", "-Wall", "-Wextra", "-Werror"]
        for local in (False, True):
            define = "#define LOCAL\n" if local else ""
            for header in ("01-template.hpp", "02-debug.hpp"):
                body = 'int main() {}'
                if header == "02-debug.hpp":
                    body = '''int main() {
                        trace("first"); trace("second");
                        if (true) debug(7); else debug(8);
                        int value = (debug(), 42);
                        return value != 42;
                    }'''
                source = define + f'#include "{ROOT / "01-Core" / header}"\n' + body
                run([*flags, "-x", "c++", "-fsyntax-only", "-"], f"standalone {header} LOCAL={local}", source)
        with tempfile.TemporaryDirectory(prefix="p002-c01-link-") as temp:
            temp = Path(temp)
            for mixed in (False, True):
                a, b, binary = temp / 'a.cpp', temp / 'b.cpp', temp / 'a.out'
                a.write_text(f'''#define LOCAL
#include "{ROOT / '01-Core/02-debug.hpp'}"
string other() {{ return Debug::to_string(std::optional<int>{{7}}); }}
''')
                define = '' if mixed else '#define LOCAL\n'
                b.write_text(define + f'''#include "{ROOT / '01-Core/02-debug.hpp'}"
string other();
int main() {{ debug(); return other() != "optional(7)"; }}
''')
                run([*flags, a, b, "-o", binary], f"multiple TU link mixed-LOCAL={mixed}")
                run([binary], f"multiple TU execution mixed-LOCAL={mixed}")
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        raise SystemExit(f"FAIL debug compile check\ncommand: {shlex.join(command)}\n{error}")


if __name__ == "__main__":
    main()
