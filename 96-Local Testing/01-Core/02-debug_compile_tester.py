"""Standalone headers, macro syntax, and ODR linkage in LOCAL/mixed builds."""
import itertools
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap
from _00_cpp_test_runner import Builder, compiler_command, timed_run

_00_memory_cap.ensure()

ROOT = Path(__file__).resolve().parents[2]


def main():
    compiler = compiler_command()
    if not compiler or shutil.which(compiler[0]) is None:
        raise SystemExit("FAIL: CXX compiler not found")
    command = []

    def run(args, label):
        nonlocal command
        command = [str(x) for x in args]
        result = timed_run(label, command, text=True, capture_output=True, timeout=120)
        if result.returncode:
            raise RuntimeError(f"{label}: exit={result.returncode}\n{result.stdout}{result.stderr}")
        print(f"PASS {label}")

    try:
        flags = ["-std=gnu++20", "-O2", "-Wall", "-Wextra", "-Werror"]
        with tempfile.TemporaryDirectory(prefix="p002-c01-link-") as temp:
            temp = Path(temp)
            builder = Builder(compiler, temp, failure=RuntimeError)
            for local, header in itertools.product((False, True), ("01-template.hpp", "02-debug.hpp")):
                define = "#define LOCAL\n" if local else ""
                body = 'int main() {}'
                if header == "02-debug.hpp":
                    body = '''int main() {
                        trace("first"); trace("second");
                        if (true) debug(7); else debug(8);
                        int value = (debug(), 42);
                        return value != 42;
                    }'''
                source = temp / f"standalone-{header}-{int(local)}.cpp"
                source.write_text(define + f'#include "{ROOT / "01-Core" / header}"\n' + body)
                builder.syntax(f"standalone {header} LOCAL={local}", flags, source, 120)
                print(f"PASS standalone {header} LOCAL={local}")
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
                builder.build(f"multiple TU link mixed-LOCAL={mixed}", flags, [a, b], binary, 120)
                print(f"PASS multiple TU link mixed-LOCAL={mixed}")
                run([binary], f"multiple TU execution mixed-LOCAL={mixed}")
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        raise SystemExit(f"FAIL debug compile check\ncommand: {shlex.join(command)}\n{error}")


if __name__ == "__main__":
    main()
