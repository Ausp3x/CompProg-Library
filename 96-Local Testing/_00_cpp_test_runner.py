from concurrent.futures import ThreadPoolExecutor, wait
import functools
import os
import platform
import re
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

TEST_DIR = Path(__file__).resolve().parent
ROOT = TEST_DIR.parent


@functools.cache
def _triple(compiler: tuple[str, ...]) -> str:
    try:
        r = subprocess.run([*compiler, "-dumpmachine"], capture_output=True, text=True, timeout=30)
        return r.stdout.strip() if r.returncode == 0 else ""
    except (OSError, subprocess.TimeoutExpired):
        return ""


def _arch(compiler: list[str] | None) -> str:
    """The compiler's (default ``$CXX`` or ``g++``) target architecture: the first field of its ``-dumpmachine`` triple."""
    return _triple(tuple(compiler or shlex.split(os.environ.get("CXX", "g++")))).split("-")[0]


_COMPILE = re.compile(r"""\[\s*(?:\*?\s*(?:compiler|cxx)\b|['"](?:g\+\+|clang\+\+)['"])|['"]-[co]['"]""")


def bypasses(text: str, contest: bool) -> list[str]:
    """What keeps a tester's C++ builds out of ``Builder``, for ``03-consistency.py``: a contest tester (``contest``) without the
    shared runner, and each line that runs the compiler other than for a rejection probe or a query, which ccache cannot take."""
    found = ["no _00_contest_runner"] if contest and "_00_contest_runner" not in text else []
    return found + [line.strip() for line in text.splitlines()
                    if _COMPILE.search(line) and not any(flag in line for flag in ("-fsyntax-only", "-dumpversion", "-dumpmachine", "-dM"))]


def runs_in_order(text: str) -> bool:
    """Whether a tester compiles its builds at once and then runs them one at a time, for ``03-consistency.py``."""
    return "build_all(" in text and "concurrently(" not in text


def x86(compiler: list[str] | None = None) -> bool:
    """Whether the compiler targets x86-64."""
    return _arch(compiler) == "x86_64"


def x86_only(flags: list[str], compiler: list[str] | None = None) -> list[str]:
    """``flags`` for an x86-64 target, else none: elsewhere the scalar path is the only path."""
    return list(flags) if x86(compiler) else []


def native(compiler: list[str] | None = None) -> bool:
    """Whether the compiler targets this host's architecture; otherwise its binaries run under an emulator (binfmt)."""
    host = platform.machine()
    return _arch(compiler) == {"AMD64": "x86_64", "arm64": "aarch64"}.get(host, host)


def scaled(seconds: int, compiler: list[str] | None = None) -> int:
    """A run timeout: seconds for native binaries, ten times that under an emulator (qemu-user is 10-50x slower)."""
    return seconds if native(compiler) else 10 * seconds


def cpu_runs(features: set[str], compiler: list[str] | None = None) -> bool:
    """Whether binaries using these x86 CPU features (``/proc/cpuinfo`` names) run here."""
    cpu = Path("/proc/cpuinfo")
    return (x86(compiler) and platform.machine() in ("x86_64", "AMD64") and cpu.exists()
            and set(features) <= set(cpu.read_text().split()))


class TestFailure(RuntimeError):
    pass


def compiler_command() -> list[str]:
    """``$CXX`` (default ``g++``, also when empty or blank) as a command prefix; it may hold arguments."""
    return shlex.split(os.environ.get("CXX", "")) or ["g++"]


@functools.cache
def _ccache_prefix(compiler: tuple[str, ...]) -> tuple[str, ...]:
    """``("ccache",)`` when a probe compile through it works; otherwise ``()`` after one SKIP line for this compiler."""
    if Path(compiler[0]).name == "ccache":  # $CXX already names it
        return ()
    if shutil.which("ccache") is None:
        print("SKIP ccache: not installed; plain builds", flush=True)
        return ()
    with tempfile.TemporaryDirectory(prefix="cp-ccache-probe-") as directory:
        (Path(directory) / "probe.cpp").write_text("int main() {}\n")
        try:
            result = subprocess.run(["ccache", *compiler, "-std=gnu++20", "-fsyntax-only", "probe.cpp"], cwd=directory,
                                    capture_output=True, text=True, timeout=120)
            reason = (result.stderr.strip().splitlines() or ["probe failed"])[0] if result.returncode else ""
        except (OSError, subprocess.TimeoutExpired) as error:
            reason = str(error)
    if reason:
        print(f"SKIP ccache: {reason}; plain builds", flush=True)
        return ()
    return ("ccache",)


def cached(compiler: list[str]) -> list[str]:
    """``compiler`` prefixed with ccache when that works. ccache caches ``-c`` and ``-fsyntax-only``, never a compile-and-link."""
    return [*_ccache_prefix(tuple(compiler)), *compiler]


def cache_env(env: dict[str, str] | None = None) -> dict[str, str]:
    """``env`` for a compile: absolute paths under the tree root hash as relative ones, so a shard or scratch copy of the tree hits."""
    return dict(os.environ if env is None else env, CCACHE_NOHASHDIR="1", CCACHE_BASEDIR=str(ROOT))


class Builder:
    """The C++ builds of one test process: each unit compiled with ``-c`` through ccache, then linked with the compiler.

    A unit inside ``directory`` (a generated source) compiles there under its relative name, any other from the tree root,
    so an unchanged unit hits in every run. A compile that must fail stays a plain command: ccache stores no failure.
    """

    def __init__(self, compiler: list[str], directory: str | Path, *, env: dict[str, str] | None = None,
                 failure: type[Exception] = TestFailure):
        self.compiler, self.directory, self.failure = compiler, Path(directory), failure
        self.prefix, self.env = cached(compiler), cache_env(env)
        self.cond, self.running, self.used, self.local = threading.Condition(), 0, 0, threading.local()
        capped = os.environ.get("CP_MEMORY_CAPPED", "")
        self.budget = int(capped) - 400 if capped.isdigit() else 1 << 40

    def _run(self, command: list[str], label: str, timeout: int, cwd: Path) -> None:
        start = time.monotonic()
        try:
            result = timed_run(label, command, cwd=cwd, env=self.env, text=True, capture_output=True, timeout=timeout,
                               encoding="utf-8", errors="replace")
        except subprocess.TimeoutExpired as error:
            raise self.failure(f"{label}: timeout={timeout}s\ncommand={shlex.join(command)}\n"
                               f"stdout={error.stdout!r}\nstderr={error.stderr!r}") from error
        if result.returncode:
            raise self.failure(f"{label}: exit={result.returncode} expected=0\ncommand={shlex.join(command)}\n"
                               f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}")
        self.local.seconds = self._spent() + time.monotonic() - start

    def _spent(self) -> float:
        """The compile seconds this thread has spent: a build's concurrent neighbours count on their own threads."""
        return getattr(self.local, "seconds", 0.0)

    def _unit(self, source: str | Path) -> tuple[str, Path]:
        path = Path(source)
        return (str(path.relative_to(self.directory)), self.directory) if path.is_relative_to(self.directory) else (str(path), ROOT)

    def compile_unit(self, label: str, options: list[str], source: str | Path, obj: str | Path, timeout: int) -> None:
        name, cwd = self._unit(source)
        self._run([*self.prefix, *options, "-c", name, "-o", str(obj)], label, timeout, cwd)

    def link(self, label: str, options: list[str], objects: list[str], output: str | Path, timeout: int) -> None:
        self._run([*self.compiler, *options, *objects, "-o", str(output)], label, timeout, ROOT)

    def build(self, label: str, options: list[str], sources: list, output: str | Path, timeout: int) -> float:
        """Compile and link ``sources`` into ``output``; returns the seconds taken."""
        start, objects = time.monotonic(), [f"{output}-{k}.o" for k in range(len(sources))]
        for source, obj in zip(sources, objects):
            self.compile_unit(label, options, source, obj, timeout)
        self.link(label, options, objects, output, timeout)
        return time.monotonic() - start

    def syntax(self, label: str, options: list[str], source: str | Path, timeout: int) -> None:
        name, cwd = self._unit(source)
        self._run([*self.prefix, *options, "-fsyntax-only", name], label, timeout, cwd)

    def stopwatch(self) -> tuple[float, float]:
        """A mark for ``report``: the time now and the compile seconds so far."""
        return time.monotonic(), self._spent()

    def report(self, mark: tuple[float, float], name: str, mode: str, compile_seconds: float = 0.0) -> None:
        """Print ``timing`` for the work since ``mark``, counting a build made during it as compile time."""
        spent = self._spent() - mark[1]
        timing(name, mode, compile_seconds + spent, time.monotonic() - mark[0] - spent)

    def admitted(self, need: int) -> "_Admission":
        """Context manager holding one of at most three build slots and ``need`` MB of the cap in force (a lone build always starts)."""
        return _Admission(self, need)

    def build_all(self, builds: list[tuple[str, str, list[str], list, str | Path, int, int]]) -> dict[str, float]:
        """Run ``(name, label, options, sources, output, timeout, MB)`` builds, at most three at once within the cap; returns
        the seconds per name and raises the first failure in ``builds`` order once all have ended."""
        def one(name: str, label: str, options: list[str], sources: list, output: str | Path, timeout: int, need: int) -> float:
            with self.admitted(need):
                return self.build(label, options, sources, output, timeout)
        futures = _await(lambda pool: [pool.submit(one, *build) for build in builds])
        return {build[0]: future.result() for build, future in zip(builds, futures)}

    def concurrently(self, jobs: list[tuple[int, object]]) -> None:
        """Run ``(MB, function)`` jobs at most three at once within the cap in force; each job's printed lines appear together,
        in job order. The first failure kills the processes of the jobs still running, skips those not started and is raised."""
        output, errors = _JobOutput(sys.stdout, len(jobs)), []

        def one(k: int, need: int, job) -> None:
            try:
                with self.admitted(need):
                    if errors or STOPPING[0]:
                        return
                    output.enter(k)
                    try:
                        job()
                    except BaseException as error:
                        with self.cond:
                            errors.append(error)
                            first = len(errors) == 1
                        if first:
                            _kill_children()
            finally:
                output.leave(k)
        sys.stdout = output
        try:
            _await(lambda pool: [pool.submit(one, k, need, job) for k, (need, job) in enumerate(jobs)])
        finally:
            sys.stdout = output.stream
        if errors:
            raise errors[0]


STOPPING = [False]  # true once the main thread is interrupted: no further test command starts (a flag, since a handler takes no lock)


def _await(submit) -> list:
    """Run ``submit(pool)`` on a three-thread pool and wait for the futures it returns. On SIGINT (Ctrl-C, ``01-run.py``),
    from before the first submission, start nothing more, kill what runs until every thread has ended, then raise
    KeyboardInterrupt; the handler only records the signal, so no exception lands inside the futures' locks."""
    hit, old = [], signal.getsignal(signal.SIGINT)
    own = threading.current_thread() is threading.main_thread() and old is signal.default_int_handler
    if own:
        signal.signal(signal.SIGINT, lambda *_: (hit.append(1), STOPPING.__setitem__(0, True)))
    pool = ThreadPoolExecutor(max_workers=3)
    try:
        futures = submit(pool)
        while wait(futures, timeout=0.1).not_done:
            if hit:  # queued jobs still run, and return at once: STOPPING stops every later command
                _kill_children()
    finally:
        if own:
            signal.signal(signal.SIGINT, old)
    pool.shutdown()
    if hit:
        raise KeyboardInterrupt
    return futures


def _kill_children() -> None:
    """SIGKILL every descendant of this process (the running builds and test binaries of a failed ``concurrently``)."""
    def descendants(pid: int | str):
        for task in Path(f"/proc/{pid}/task").glob("*"):
            try:
                children = (task / "children").read_text().split()
            except OSError:
                continue
            for child in children:
                yield from descendants(child)
                yield int(child)
    for pid in list(descendants(os.getpid())):
        try:
            os.kill(pid, signal.SIGKILL)
        except ProcessLookupError:
            pass


class _JobOutput:
    """``sys.stdout`` during ``Builder.concurrently``: the first unfinished job writes through, the others hold their lines until their turn."""

    def __init__(self, stream, jobs: int):
        self.stream, self.held, self.finished, self.head = stream, [[] for _ in range(jobs)], [False] * jobs, 0
        self.lock, self.owner = threading.Lock(), {}

    def enter(self, k: int) -> None:
        self.owner[threading.get_ident()] = k

    def write(self, text: str) -> int:
        with self.lock:
            k = self.owner.get(threading.get_ident(), self.head)
            if k == self.head:
                self.stream.write(text)
            else:
                self.held[k].append(text)
        return len(text)

    def flush(self) -> None:
        self.stream.flush()

    def isatty(self) -> bool:
        return False

    def leave(self, k: int) -> None:
        with self.lock:
            self.finished[k] = True
            while self.head < len(self.finished) and self.finished[self.head]:
                self.head += 1
                if self.head < len(self.finished):
                    self.stream.write("".join(self.held[self.head]))
                    self.held[self.head].clear()
            self.stream.flush()


class _Admission:
    def __init__(self, builder: Builder, need: int):
        self.builder, self.need = builder, need

    def __enter__(self) -> None:
        b = self.builder
        with b.cond:
            b.cond.wait_for(lambda: b.running == 0 or b.running < 3 and b.used + self.need <= b.budget)
            b.used += self.need
            b.running += 1

    def __exit__(self, *_) -> None:
        b = self.builder
        with b.cond:
            b.used -= self.need
            b.running -= 1
            b.cond.notify_all()


def over_half(label: str, seconds: float, limit: float) -> None:
    """Print the line ``01-run.py`` lists as a problem to fix: a command that took over half its limit (05-testing.md, Layout and modes)."""
    if seconds * 2 > limit:
        print(f"OVERHALF {label}: {seconds:.0f} s of its {limit:.0f} s limit\n", end="", flush=True)


def timed_run(label: str, command: list[str], *, timeout: float, **options) -> subprocess.CompletedProcess:
    """``subprocess.run`` under a limit, reporting a command over half of it; none starts after an interrupt (``STOPPING``)."""
    if STOPPING[0]:
        raise KeyboardInterrupt
    start = time.monotonic()
    result = subprocess.run(command, timeout=timeout, **options)
    if result.returncode == -signal.SIGINT:  # Ctrl-C reached the command before the main thread's handler ran
        STOPPING[0] = True
        raise KeyboardInterrupt
    over_half(label, time.monotonic() - start, timeout)
    return result


def timing(name: str, mode: str, compile_seconds: float, run_seconds: float, extra: str = "") -> None:
    """Print one build's times: ``01-run.py --mode quick`` lists the builds whose run exceeds 10 s (05-testing.md, Layout and modes)."""
    print(f"TIMING {name} mode={mode} compile={compile_seconds:.2f}s run={run_seconds:.2f}s{extra}", flush=True)
