#!/usr/bin/env python3
"""Bounded problem-specific shrinking; keep beside 01-stress.py for process cleanup."""
import argparse
import math
import os
from pathlib import Path
import runpy
import shlex
import sys
import tempfile
import uuid


# Loading the sibling defines its helpers without invoking its CLI.
invoke = runpy.run_path(str(Path(__file__).resolve().with_name("01-stress.py")))["invoke"]


def cmd(path):
    path = Path(path).resolve()
    if not path.is_file():
        raise ValueError(f"missing hook: {path}")
    if path.suffix == ".py":
        return [sys.executable, str(path)]
    if not os.access(path, os.X_OK):
        raise ValueError(f"hook is not executable: {path}")
    return [str(path)]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("input", type=Path, help="existing failing input")
    p.add_argument("--propose", required=True, help="hook input step -> proposed input on stdout; exit 3 to skip")
    p.add_argument("--valid", required=True, help="hook input -> exit 0 valid, 1 invalid")
    p.add_argument("--probe", required=True, help="hook input -> exact failure signature on stdout; exit 0")
    p.add_argument("--max-steps", type=int, default=100)
    p.add_argument("--timeout", type=float, default=2)
    p.add_argument("--output", type=Path, help="new reduced-input file; also saved after a hook error or interrupt")
    args = p.parse_args()
    if args.max_steps <= 0 or not math.isfinite(args.timeout) or args.timeout <= 0:
        p.error("max-steps and timeout must be positive; timeout must be finite")
    try:
        hooks = {k: cmd(getattr(args, k)) for k in ("propose", "valid", "probe")}
        original = args.input.resolve().read_bytes()
        output = args.output or args.input.with_name(f"shrunk-{uuid.uuid4().hex[:8]}.in")
        if output.exists() or output.is_symlink():
            raise ValueError(f"output already exists: {output}")

        def run(role, label, *extra, allowed=("ok",)):
            command = hooks[role] + [str(path), *extra]
            status, out, err = invoke(command, b"", args.timeout)
            if status not in allowed:
                raise ValueError(f"{label}: {status}; command={shlex.join(command)}; stderr={err!r}")
            return status, out

        with tempfile.TemporaryDirectory(prefix="contest-shrink-") as tmp:
            path = Path(tmp) / "current.in"
            path.write_bytes(original)
            status, _ = run("valid", "initial validator", allowed=("ok", "exit 1"))
            if status != "ok":
                raise ValueError("initial input must be valid")
            _, signature = run("probe", "initial probe")
            if not signature.strip():
                raise ValueError("initial input must have a nonempty failure signature")
            current, accepted, error = original, 0, None
            try:
                for step in range(args.max_steps):
                    path.write_bytes(current)
                    status, candidate = run("propose", f"proposer at step {step}", str(step),
                                            allowed=("ok", "exit 3"))
                    if status == "exit 3" or len(candidate) >= len(current):
                        continue
                    path.write_bytes(candidate)
                    status, _ = run("valid", f"validator at step {step}", allowed=("ok", "exit 1"))
                    if status == "exit 1":
                        continue
                    _, tested = run("probe", f"probe at step {step}")
                    if tested == signature:
                        current, accepted = candidate, accepted + 1
            except (OSError, ValueError, KeyboardInterrupt) as e:
                error = e
        # The original remains untouched; even an interrupted search retains its best case.
        with output.open("xb") as f:
            f.write(current)
        print(f"{args.input} -> {output}: {len(original)} -> {len(current)} bytes; "
              f"{accepted} accepted; signature={signature!r}", flush=True)
        if error is not None:
            print(f"saved: {output}", file=sys.stderr)
            if isinstance(error, KeyboardInterrupt):
                print("interrupted", file=sys.stderr)
                return 130
            p.exit(2, f"shrink error: {error}\n")
        return 0
    except (OSError, ValueError) as e:
        p.exit(2, f"shrink error: {e}\n")
    except KeyboardInterrupt:
        print("interrupted", file=sys.stderr)
        return 130


if __name__ == "__main__":
    sys.exit(main())
