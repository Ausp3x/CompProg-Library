#!/usr/bin/env python3
"""Small contest stress runner. See 00-index.md for command and hook contracts."""
import argparse
import base64
import itertools
import json
import math
import os
from pathlib import Path
import select
import signal
import shlex
import subprocess
import sys
import tempfile
import threading
import time
import uuid
from datetime import datetime
from decimal import Decimal, DecimalException, MAX_EMAX, MIN_EMIN, Underflow, localcontext


def command(path, python, cxx, flags, build):
    path = Path(path).resolve()
    if not path.is_file():
        raise ValueError(f"missing program: {path}")
    if path.suffix == ".cpp":
        binary = build / (path.stem + "-" + uuid.uuid4().hex[:8])
        cmd = [cxx, *shlex.split(flags), str(path), "-o", str(binary)]
        result = invoke(cmd, b"", 60)
        if result[0] != "ok":
            raise ValueError(f"compile failed: {shlex.join(cmd)}\n{result[2].decode(errors='replace')}")
        return [str(binary)]
    if path.suffix == ".py":
        return [python, str(path)]
    if not os.access(path, os.X_OK):
        raise ValueError(f"program is not executable: {path}")
    return [str(path)]


def invoke(cmd, data, timeout):
    try:
        p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, start_new_session=True)
        try:
            out, err = p.communicate(input=data, timeout=timeout)
        except subprocess.TimeoutExpired as e:
            kill_group(p)
            try:
                out, err = p.communicate(timeout=1)
            except subprocess.TimeoutExpired:
                out, err = e.stdout or b"", e.stderr or b""
                p.stdout.close()
                p.stderr.close()
            return "timeout", out, err
        return ("ok" if p.returncode == 0 else f"exit {p.returncode}", out, err)
    except OSError as e:
        return (f"launch error: {e}", b"", str(e).encode())
    finally:
        if "p" in locals():
            kill_group(p)
            p.wait()
            for pipe in (p.stdin, p.stdout, p.stderr):
                if pipe:
                    pipe.close()


def kill_group(p):
    try:
        os.killpg(p.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    if p.poll() is None:
        try:
            p.kill()
        except ProcessLookupError:
            pass


def save_failure(root, seed, kind, data, runs, commands, sources, config, extra=None):
    if root.is_symlink():
        raise ValueError(f"artifact root is a symlink: {root}")
    root.mkdir(parents=True, exist_ok=True, mode=0o700)
    name = datetime.now().strftime("%Y%m%d-%H%M%S-%f") + f"-seed{seed}-{uuid.uuid4().hex[:6]}"
    folder = root / name
    folder.mkdir()
    (folder / "input.txt").write_bytes(data)
    meta = {"seed": seed, "kind": kind, "commands": commands,
            "sources": sources, "config": config,
            "statuses": {key: val[0] for key, val in runs.items()}}
    if extra:
        meta.update(extra)
    (folder / "case.json").write_text(json.dumps(meta, indent=2, default=str) + "\n")
    for key, (_, out, err) in runs.items():
        (folder / f"{key}.out").write_bytes(out)
        (folder / f"{key}.err").write_bytes(err)
    return folder


def judged_status(result, role):
    return None if result[0] == "ok" else f"{role}_{'timeout' if result[0] == 'timeout' else 'crash'}"


def validate_input(args, data, cmds, case_dir, runs):
    case_dir.mkdir(parents=True, exist_ok=True)
    path = case_dir / "input.txt"
    path.write_bytes(data)
    runs["input_validator"] = status = invoke(cmds["input_validator"] + [str(path)], b"", args.timeout)
    return None if status[0] == "ok" else "invalid_input" if status[0] == "exit 1" else "input_validator_error"


def check_batch(args, data, cmds, case_dir):
    runs = {}
    if args.expected:
        runs["reference"] = ("ok", args.expected_bytes, b"")
    elif "reference" in cmds:
        runs["reference"] = invoke(cmds["reference"], data, args.timeout)
        if runs["reference"][0] != "ok":
            return "reference_error", runs, {}
    runs["candidate"] = invoke(cmds["candidate"], data, args.timeout)
    bad = judged_status(runs["candidate"], "candidate")
    if bad:
        return bad, runs, {}
    if args.checker:
        case_dir.mkdir(parents=True, exist_ok=True)
        paths = [case_dir / "input.txt", case_dir / "candidate.txt", case_dir / "reference.txt"]
        for path, content in zip(paths, (data, runs["candidate"][1], runs.get("reference", ("", b"", b""))[1])):
            path.write_bytes(content)
        runs["checker"] = invoke(cmds["checker"] + list(map(str, paths)), b"", args.timeout)
        code = runs["checker"][0]
        return ("pass" if code == "ok" else "wrong_answer" if code == "exit 1" else "checker_error"), runs, {}
    same = (runs["candidate"][1] == runs["reference"][1] if args.exact
            else runs["candidate"][1].split() == runs["reference"][1].split())
    return ("pass" if same else "wrong_answer"), runs, {}


def score(args, data, cmds, case_dir):
    runs = {}
    case_dir.mkdir(parents=True, exist_ok=True)
    input_path = case_dir / "input.txt"
    input_path.write_bytes(data)
    values = {}
    for role in ("reference", "candidate") if "reference" in cmds else ("candidate",):
        runs[role] = invoke(cmds[role], data, args.timeout)
        bad = judged_status(runs[role], role)
        if bad:
            return ("reference_error" if role == "reference" else bad), runs, values
        output = case_dir / f"{role}.txt"
        output.write_bytes(runs[role][1])
        runs[f"{role}_validator"] = invoke(cmds["validator"] + [str(input_path), str(output)], b"", args.timeout)
        if runs[f"{role}_validator"][0] != "ok":
            kind = "invalid_output" if runs[f"{role}_validator"][0] == "exit 1" else "validator_error"
            return ("reference_error" if role == "reference" else kind), runs, values
        runs[f"{role}_scorer"] = invoke(cmds["scorer"] + [str(input_path), str(output)], b"", args.timeout)
        raw = runs[f"{role}_scorer"][1].decode(errors="replace").split()
        try:
            if runs[f"{role}_scorer"][0] != "ok" or len(raw) != 1:
                raise ValueError()
            value = Decimal(raw[0])
            if not value.is_finite():
                raise ValueError()
        except (ValueError, DecimalException):
            return "scorer_error", runs, values
        values[role] = value
    if "reference" in values:
        # Keep all input significant digits; a widely separated exponent may
        # still round the gap. The sign always records the exact ordering.
        try:
            with localcontext() as context:
                context.prec = max(28, *(len(v.as_tuple().digits) + 1 for v in values.values()))
                context.Emax, context.Emin = MAX_EMAX, MIN_EMIN
                context.traps[Underflow] = True
                values["gap"] = (values["reference"] - values["candidate"] if args.objective == "maximize"
                                 else values["candidate"] - values["reference"])
        except DecimalException:
            return "scorer_error", runs, values
    return "pass", runs, values


def interactive(args, data, cmds, case_dir):
    case_dir.mkdir(parents=True, exist_ok=True)
    input_path = case_dir / "input.txt"
    input_path.write_bytes(data)
    transcript = case_dir / "transcript.jsonl"
    log = transcript.open("w")
    runs = {}
    procs = {}
    stderr_files = {}
    try:
        for role, cmd in (("candidate", cmds["candidate"]),
                          ("interactor", cmds["interactor"] + [str(input_path)])):
            stderr_files[role] = (case_dir / f"{role}.err").open("wb")
            procs[role] = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                           stderr=stderr_files[role], bufsize=0,
                                           start_new_session=True)
    except (OSError, KeyboardInterrupt) as e:
        runs[role] = (f"launch error: {e}", b"", str(e).encode())
        for p in procs.values():
            kill_group(p)
        for started, p in procs.items():
            try:
                out, _ = p.communicate(timeout=1)
            except subprocess.TimeoutExpired as expired:
                out = expired.stdout or b""
            p.wait()
            p.stdin.close()
            p.stdout.close()
            runs[started] = ("ok" if p.returncode == 0 else f"exit {p.returncode}",
                             out, (case_dir / f"{started}.err").read_bytes())
        for f in stderr_files.values():
            f.close()
        log.close()
        if isinstance(e, KeyboardInterrupt):
            raise
        return "launch_error", runs, {}
    lock = threading.Lock()
    stopped = threading.Event()
    errors = []
    activity = [time.monotonic()]

    def pump(source, target, label):
        try:
            os.set_blocking(source.fileno(), False)
            os.set_blocking(target.fileno(), False)
            with (case_dir / f"{label}.bin").open("wb") as raw:
                while not stopped.is_set():
                    if not select.select([source], [], [], 0.05)[0]:
                        continue
                    chunk = os.read(source.fileno(), 4096)
                    if not chunk:
                        break
                    with lock:
                        activity[0] = time.monotonic()
                        raw.write(chunk)
                        log.write(json.dumps({"seconds": round(activity[0] - start, 6),
                                              "direction": label,
                                              "base64": base64.b64encode(chunk).decode()}) + "\n")
                        log.flush()
                    view = memoryview(chunk)
                    while view and not stopped.is_set():
                        if not select.select([], [target], [], 0.05)[1]:
                            continue
                        try:
                            view = view[os.write(target.fileno(), view):]
                        except BrokenPipeError:
                            break
        except OSError as e:
            with lock:
                errors.append(str(e))
        finally:
            target.close()
            source.close()

    start = time.monotonic()
    threads = [threading.Thread(target=pump, args=(procs["candidate"].stdout, procs["interactor"].stdin, "candidate_to_interactor")),
               threading.Thread(target=pump, args=(procs["interactor"].stdout, procs["candidate"].stdin, "interactor_to_candidate"))]
    failure = None
    live_on_timeout = set()
    try:
        for t in threads:
            t.start()
        while any(p.poll() is None for p in procs.values()) or any(t.is_alive() for t in threads):
            now = time.monotonic()
            with lock:
                idle = now - activity[0]
                if errors:
                    failure = "io_error"
                    break
            if now - start > args.whole_timeout:
                failure = "whole_timeout"
                live_on_timeout = {role for role, p in procs.items() if p.poll() is None}
                break
            if idle > args.idle_timeout:
                failure = "idle_timeout"
                live_on_timeout = {role for role, p in procs.items() if p.poll() is None}
                break
            time.sleep(0.01)
    except KeyboardInterrupt:
        failure = "interrupted"
        raise
    except RuntimeError as e:
        errors.append(str(e))
        failure = "io_error"
    finally:
        for p in procs.values():
            kill_group(p)
        stopped.set()
        for p in procs.values():
            p.wait()
        for t in threads:
            if t.ident is not None:
                t.join()
        for p in procs.values():
            p.stdin.close()
            p.stdout.close()
        log.close()
        for f in stderr_files.values():
            f.close()
    if errors:
        failure = "io_error"
    for role, p in procs.items():
        err = (case_dir / f"{role}.err").read_bytes()
        runs[role] = ("ok" if p.returncode == 0 else f"exit {p.returncode}", b"", err)
    if failure == "io_error":
        runs["relay"] = ("io error", b"", "\n".join(errors).encode())
        kind = "interactive_io_error"
    elif failure and "interactor" not in live_on_timeout and runs["interactor"][0] != "ok":
        kind = "interactor_rejected" if runs["interactor"][0] == "exit 1" else "interactor_error"
    elif failure and "candidate" not in live_on_timeout and runs["candidate"][0] != "ok":
        kind = "candidate_crash"
    elif failure:
        prefix = ("interactive_" if len(live_on_timeout) != 1 else
                  "candidate_" if "candidate" in live_on_timeout else "interactor_")
        kind = prefix + failure
    elif runs["interactor"][0] != "ok":
        kind = "interactor_rejected" if runs["interactor"][0] == "exit 1" else "interactor_error"
    elif runs["candidate"][0] != "ok":
        kind = "candidate_crash"
    else:
        kind = "pass"
    return kind, runs, {}


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("generator", help="program taking seed as first argument, writing complete input to stdout")
    p.add_argument("candidate")
    p.add_argument("reference", nargs="?", help="brute/reference for batch (optional with checker) or scored mode")
    p.add_argument("--mode", choices=("batch", "scored", "interactive"), default="batch")
    p.add_argument("--count", type=int, default=0, help="0 = run until failure or Ctrl-C")
    p.add_argument("--seed", type=int, default=1)
    p.add_argument("--replay", type=Path, help="run one saved input; generator is ignored")
    p.add_argument("--timeout", type=float, default=2, help="seconds per batch process")
    p.add_argument("--whole-timeout", type=float, default=10)
    p.add_argument("--idle-timeout", type=float, default=2)
    p.add_argument("--python", default=sys.executable)
    p.add_argument("--cxx", default="g++")
    p.add_argument("--cxxflags", default="-O2 -std=gnu++20")
    p.add_argument("--exact", action="store_true", help="compare exact bytes instead of whitespace tokens")
    p.add_argument("--expected", type=Path, help="batch replay: saved answer file used as the reference output")
    p.add_argument("--input-validator", help="any mode: input path; 0 valid, 1 invalid input, judged before programs run")
    p.add_argument("--checker", help="batch hook: input candidate_output reference_output; 0 accepted, 1 wrong")
    p.add_argument("--interactor", help="interactive program receives hidden input file as first argument")
    p.add_argument("--validator", help="scored hook: input output; 0 valid, 1 invalid")
    p.add_argument("--scorer", help="scored hook: input output; prints one finite number")
    p.add_argument("--objective", choices=("maximize", "minimize"), default="maximize")
    p.add_argument("--artifacts", type=Path, default=Path(tempfile.gettempdir()) / "contest-stress-artifacts")
    args = p.parse_args(argv)
    if args.count < 0 or any(not math.isfinite(t) or t <= 0
                             for t in (args.timeout, args.whole_timeout, args.idle_timeout)):
        p.error("count must be nonnegative and timeouts finite and positive")
    if args.mode == "batch" and not args.reference and not args.checker and not args.expected:
        p.error("batch mode needs a reference program, --expected or --checker")
    if args.expected and (args.mode != "batch" or not args.replay or args.reference):
        p.error("--expected needs batch mode, --replay and no reference program")
    if args.mode == "scored" and (not args.validator or not args.scorer):
        p.error("scored mode needs --validator and --scorer")
    if args.mode == "interactive" and not args.interactor:
        p.error("interactive mode needs --interactor")
    if args.mode == "interactive" and args.reference:
        p.error("interactive mode does not use a reference program")
    if args.mode != "batch" and (args.checker or args.exact):
        p.error("--checker and --exact need batch mode")
    if args.checker and args.exact:
        p.error("--exact compares against the reference; a --checker judges instead")
    if args.mode != "scored" and (args.validator or args.scorer):
        p.error("--validator and --scorer need scored mode")
    if args.mode != "interactive" and args.interactor:
        p.error("--interactor needs interactive mode")
    with tempfile.TemporaryDirectory(prefix="contest-stress-build-") as tmp:
        build = Path(tmp)
        try:
            sources = {"candidate": args.candidate}
            if not args.replay:
                sources["generator"] = args.generator
            if args.reference:
                sources["reference"] = args.reference
            for key in ("input_validator", "checker", "validator", "scorer", "interactor"):
                value = getattr(args, key)
                if value:
                    sources[key] = value
            sources = {key: str(Path(value).resolve()) for key, value in sources.items()}
            cmds = {key: command(value, args.python, args.cxx, args.cxxflags, build)
                    for key, value in sources.items()}
            args.expected_bytes = args.expected.read_bytes() if args.expected else None
        except (ValueError, OSError) as e:
            p.exit(2, str(e) + "\n")
        except KeyboardInterrupt:
            print("interrupted", file=sys.stderr)
            return 130
        try:
            n = 1 if args.replay else args.count
            for i in range(n) if n else itertools.count():
                seed = args.seed + i
                try:
                    data = args.replay.read_bytes() if args.replay else b""
                except OSError as e:
                    p.exit(2, f"cannot read replay input: {e}\n")
                runs = {}
                if not args.replay:
                    runs["generator"] = invoke(cmds["generator"] + [str(seed)], b"", args.timeout)
                    data = runs["generator"][1]
                case_dir = build / "case"
                if "generator" in runs and runs["generator"][0] != "ok":
                    kind, more, extra = "generator_error", {}, {}
                elif args.input_validator and (kind := validate_input(args, data, cmds, case_dir, runs)):
                    more, extra = {}, {}
                elif args.mode == "batch":
                    kind, more, extra = check_batch(args, data, cmds, case_dir)
                elif args.mode == "scored":
                    kind, more, extra = score(args, data, cmds, case_dir)
                else:
                    kind, more, extra = interactive(args, data, cmds, case_dir)
                runs.update(more)
                note = " " + " ".join(f"{k}={v}" for k, v in extra.items() if k != "transcript") if extra else ""
                print(f"seed={seed} {kind}{note}", flush=True)
                if kind != "pass":
                    comparison = (args.mode if args.mode != "batch" else
                                  "checker" if args.checker else "bytes" if args.exact else "tokens")
                    config = {"mode": args.mode, "comparison": comparison,
                              "timeout": args.timeout, "whole_timeout": args.whole_timeout,
                              "idle_timeout": args.idle_timeout, "objective": args.objective,
                              "python": args.python, "cxx": args.cxx, "cxxflags": args.cxxflags,
                              "cwd": str(Path.cwd()), "count": args.count,
                              "replay": str(args.replay.resolve()) if args.replay else None,
                              "expected": str(args.expected.resolve()) if args.expected else None}
                    executed = dict(cmds)
                    if "generator" in executed:
                        executed["generator"] = executed["generator"] + [str(seed)]
                    if "input_validator" in runs:
                        executed["input_validator"] = cmds["input_validator"] + [str(case_dir / "input.txt")]
                    if "checker" in runs:
                        executed["checker"] = cmds["checker"] + [str(case_dir / name) for name in
                                                                  ("input.txt", "candidate.txt", "reference.txt")]
                    if args.mode == "interactive":
                        executed["interactor"] = cmds["interactor"] + [str(case_dir / "input.txt")]
                    for role in ("candidate", "reference"):
                        for hook in ("validator", "scorer"):
                            key = f"{role}_{hook}"
                            if key in runs:
                                executed[key] = cmds[hook] + [str(case_dir / "input.txt"),
                                                              str(case_dir / f"{role}.txt")]
                    try:
                        folder = save_failure(args.artifacts.absolute(), seed, kind, data, runs,
                                              executed, sources, config, extra)
                        if args.mode == "interactive" and case_dir.exists():
                            for file in case_dir.iterdir():
                                if file.name.startswith(("candidate_to_", "interactor_to_", "transcript")):
                                    (folder / file.name).write_bytes(file.read_bytes())
                    except (OSError, ValueError) as e:
                        p.exit(2, f"cannot save failure artifacts: {e}\n")
                    print(f"saved: {folder}", file=sys.stderr)
                    return 1
                # Avoid old transcript/output files affecting the next case.
                for file in case_dir.iterdir() if case_dir.exists() else ():
                    file.unlink()
        except KeyboardInterrupt:
            print("interrupted", file=sys.stderr)
            return 130
        except OSError as e:
            p.exit(2, f"contest runner I/O error: {e}\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
