"""C01's deterministic GNU++20 feature/configuration runner; artifacts stay in /tmp."""
import argparse
import os
from pathlib import Path
import re
import resource
import signal
import shlex
import shutil
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def run_suite(name, sources, compile_check=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("quick", "full", "stress"), default=os.getenv("CP_TEST_MODE", "full"))
    parser.add_argument("--seed", type=int, default=int(os.getenv("CP_TEST_SEED", "335597")))
    args = parser.parse_args()
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    compiler = shutil.which(os.getenv("CXX", "g++"))
    if not compiler:
        parser.error("CXX compiler not found")
    configs = [("optimized-NDEBUG", ["-O2", "-DNDEBUG"])]
    if args.mode != "quick":
        configs += [("checked", ["-O0", "-g", "-D_GLIBCXX_ASSERTIONS"]),
                    ("ASan-UBSan", ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"])]
    env = dict(os.environ, CP_TEST_SEED=str(args.seed), CP_TEST_ITERATIONS=str({"quick": 500, "full": 5000, "stress": 30000}[args.mode]),
               ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    command = []
    done = 0
    total = 2 * len(configs) * len(sources) + (3 if args.mode != "quick" and "02-debug_tester.cpp" in sources else 0) + bool(compile_check)
    terminal = sys.stdout.isatty()
    color = terminal and "NO_COLOR" not in os.environ

    def status(message, passed=True):
        if color:
            message = ("\033[1;32m" if passed else "\033[1;31m") + message + "\033[0m"
        return message

    def execute(cmd, label, expected=0):
        nonlocal command, done
        command = list(map(str, cmd))
        if terminal:
            print(f"\r[{done}/{total}] {label}\033[K", end="", flush=True)
        result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, text=True, timeout=180)
        if (expected == 0 and result.returncode != 0) or (expected != 0 and result.returncode != expected):
            raise RuntimeError(f"{label}: exit={result.returncode}, expected={expected}\n{result.stdout}{result.stderr}")
        done += 1
        line = status(f"PASS {name}: [{done}/{total}] {label}")
        print(("\r" if terminal else "") + line + ("\033[K" if terminal else ""), end="" if terminal else "\n", flush=True)

    print(f"{name}: mode={args.mode} seed={args.seed} iterations={env['CP_TEST_ITERATIONS']}", flush=True)
    try:
        with tempfile.TemporaryDirectory(prefix="p002-c01-") as temp:
            for config, flags in configs:
                for source in sources:
                    binary = Path(temp) / f"{Path(source).stem}-{config}"
                    execute([compiler, "-std=gnu++20", "-Wall", "-Wextra", "-Wshadow", "-Wconversion", *flags, HERE / source, "-o", binary], f"{config} compile {source}")
                    execute([binary], f"{config} run {source}")
                    if source == "02-debug_tester.cpp" and config == "checked":
                        for case in ("negative-start", "inverted-range", "tracer-depth"):
                            execute([binary, case], f"{config} precondition {case}", expected=-signal.SIGABRT)
            if compile_check:
                execute([sys.executable, HERE / compile_check], "standalone/multiple-TU/macro compilation")
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        message = str(error)
        if not sys.stderr.isatty():
            message = re.sub(r"\x1b\[[0-9;]*m", "", message)
        if terminal:
            print()
        failure = f"FAIL {name}: mode={args.mode} seed={args.seed}\ncommand: {shlex.join(command)}\n{message}"
        if sys.stderr.isatty() and "NO_COLOR" not in os.environ:
            failure = "\033[1;31m" + failure + "\033[0m"
        print(failure, file=sys.stderr)
        raise SystemExit(1)
    if terminal:
        print()
    print(status(f"PASS {name}: all {args.mode} configurations"), flush=True)
