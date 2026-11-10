#!/usr/bin/env python3
"""Mutation campaign on one library header with mewt (rules: 05-testing.md, Mutation testing; evidence: Docs/00-mutation.md).

  run HEADER        copy the working tree into scratch shards (mewt mutates its target in place), generate the header's
                    mutants and select the sample: the lines changed since the blob the evidence record names (every line
                    without a record, or with --whole), high and medium severity only on a Core full header's whole-file
                    campaign, then a seeded uniform --sample of a whole-file campaign. Each stage tests the sampled
                    mutants not yet caught: tester quick mode on the optimized build, then full mode on the optimized
                    build, then full mode on the builds not yet run in full (quick then full when the tester cannot select a build). A test
                    gets 3x its stage baseline's time (at least 60 s) and runs in its own memory-capped scope (3x the
                    baseline's peak, at least 1024 MB, at most the tester's cap); as many shards run at once as
                    --memory-mb holds, each taking every width-th mutant of a seeded shuffle, and a mutant that timed out
                    is retested before its timeout counts, side by side only as far as each test keeps its own cores
                    (the shards, at most half the logical CPUs, divided by the builds a test runs at once). One campaign runs per machine (a lock in
                    $XDG_RUNTIME_DIR, or CP_MUTATION_LOCK). Every shard's header is compared with the checkout after each
                    stage. A complete run prints the
                    counts, the survivors and the record line for the evidence document and stores the header's blob in
                    git's object store (for later diff scopes). The run stays within --budget minutes; the shards stay in
                    --workdir (locked while a run uses it), and a rerun with the same --workdir tests what is untested and
                    retests the survivors with the current tester, even once the record names the current blob. SKIP
                    without mewt MEWT_VERSION. Exit 0 complete, 3 stopped by the budget, 1 failure, 130 interrupted.
  check HEADER...   exit 1 unless each header's evidence document has a `## Mutation testing` record naming the header's
                    current git blob with 0 untested mutants; --existing passes headers without a record (/prune,
                    /restyle); --changed adds every header changed against BASE (default master), committed, uncommitted
                    or untracked. /finish and /merge-pr call stale() on their own lists of changed headers.
  install           download the pinned mewt release, verify its SHA-256, unpack it into PREFIX/opt/mewt-MEWT_VERSION and
                    link it as PREFIX/bin/mewt (default PREFIX ~/.local); on arm64, which has no release binary, or with
                    --source, build the pinned source archive there with cargo --locked (Rust RUST_FLOOR or newer).
"""
import argparse
from datetime import date
import difflib
import fcntl
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import random
import re
import shlex
import shutil
import signal
import subprocess
import sys
import tarfile
import tempfile
import time
import urllib.request

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(HERE))
import _00_memory_cap  # noqa: E402

MEWT_VERSION = '4.0.0'  # AGPL-3.0: installed outside the repository, never vendored
MEWT_URL = f'https://github.com/trailofbits/mewt/releases/download/v{MEWT_VERSION}/mewt-x86_64-unknown-linux-gnu.tar.xz'
MEWT_SHA256 = '0e30678a7d090d112b9cf4d2d620053ca9da7c458ed5c2d94010304edded728b'
MEWT_SOURCE_URL = f'https://github.com/trailofbits/mewt/releases/download/v{MEWT_VERSION}/source.tar.gz'  # arm64 Linux: no binary
MEWT_SOURCE_SHA256 = 'a18b9c7d56e65f426c3d0013a51199ed42eced6d745de537b01825ee9400e3d7'
RUST_FLOOR = (1, 85)  # mewt 4.0.0 is edition 2024
HEADER = re.compile(r'0[1-7]-[^/]+/(?!9[89]-)\d{2}-[^/]+\.hpp')  # library headers, aggregates excluded
CORE_FULL = re.compile(r'01-Core/\d{2}-(?![^/]*mini)[^/]*\.hpp')
SKIPPED = ('OLD/', '95-Resources/')  # never included by a tester; the rest of the working tree is copied
RECORD = re.compile(r'blob `([0-9a-f]{40})`')
CAUGHT = ('TestFail', 'Timeout')
SEVERITIES = ('high', 'medium', 'low')


class Failure(RuntimeError):
    pass


def blob(data):
    """The git blob id of data as stored (git hash-object --no-filters without git)."""
    return hashlib.sha1(b'blob %d\0' % len(data) + data).hexdigest()


def current(root, header):
    """The git blob id of the header as git would store it (line-ending attributes applied)."""
    return git(root, 'hash-object', '--', header, raw=True).decode().strip()


def evidence(header):
    """'02-Data Structures/01-dsu.hpp' -> '02-Data Structures/Docs/01-dsu.md'."""
    folder, name = header.split('/', 1)
    return f'{folder}/Docs/{Path(name).stem}.md'


def record(root, header):
    """(blob, untested count or None) of the record line under the `## Mutation testing` heading of the header's
    evidence document, or None."""
    doc = Path(root) / evidence(header)
    text = doc.read_text(errors='replace') if doc.is_file() else ''
    section = re.search(r'^## Mutation testing[ \t]*\n(.*?)(?=^## |\Z)', text, re.M | re.S)
    line = next((l for l in section[1].splitlines() if RECORD.search(l)), None) if section else None
    if line is None:
        return None
    untested = re.search(r'(\d+) untested', line)
    return RECORD.search(line)[1], int(untested[1]) if untested else None


def recorded(root, header):
    """The blob of the header's record, or None."""
    found = record(root, header)
    return found and found[0]


def stale(root, headers, existing=False):
    """(header, reason) for every header whose record is missing (unless existing), names another blob or leaves
    mutants untested."""
    out = []
    for h in headers:
        found, now = record(root, h), current(root, h)
        if found is None:
            if not existing:
                out.append((h, f'no `## Mutation testing` record with a blob in {evidence(h)}'))
        elif found[0] != now:
            out.append((h, f'{evidence(h)} records blob {found[0][:12]}, the header is {now[:12]}'))
        elif found[1] != 0:
            out.append((h, f'the record in {evidence(h)} is incomplete ({"no" if found[1] is None else found[1]} untested count)'))
    return out


def library_path(arg):
    """arg as a path relative to ROOT: absolute, relative to ROOT when that file exists, else to the working directory."""
    p = Path(arg)
    if not p.is_absolute() and (ROOT / p).is_file():
        return p.as_posix()
    return Path(os.path.relpath(p.resolve(), ROOT)).as_posix()


def git(root, *args, raw=False):
    r = subprocess.run(['git', *args], cwd=root, capture_output=True)
    if r.returncode:
        raise Failure(f'git {" ".join(args)}: {r.stderr.decode(errors="replace").strip()}')
    return r.stdout if raw else [p for p in r.stdout.decode().split('\0') if p]


def changed(root, base='master'):
    """Headers changed against base: committed since the merge base, uncommitted or untracked, and still present."""
    paths = {*git(root, 'diff', '--name-only', '--no-renames', '-z', f'{base}...HEAD'),
             *git(root, 'diff', '--name-only', '--no-renames', '-z', 'HEAD'), *git(root, 'ls-files', '-o', '--exclude-standard', '-z')}
    return sorted(p for p in paths if HEADER.fullmatch(p) and (Path(root) / p).is_file())


def changed_lines(old, new):
    """1-based lines of new that differ from old; a pure deletion marks the lines on both sides of the gap."""
    a, b = old.decode(errors='replace').splitlines(), new.decode(errors='replace').splitlines()
    lines = set()
    for tag, _, _, j1, j2 in difflib.SequenceMatcher(None, a, b, autojunk=False).get_opcodes():
        if tag in ('replace', 'insert'):
            lines.update(range(j1 + 1, j2 + 1))
        elif tag == 'delete':
            lines.update(x for x in (j1, j1 + 1) if 1 <= x <= len(b))
    return lines


def install(prefix, url=MEWT_URL, digest=MEWT_SHA256, build=False):
    """Download and verify the pinned release, then link PREFIX/bin/mewt: the binary archive's regular files flattened
    into PREFIX/opt/mewt-MEWT_VERSION, or with build (arm64, which has no release binary) the source archive built there
    by cargo --locked."""
    prefix = Path(prefix).expanduser().resolve()
    dest, link = prefix / 'opt' / f'mewt-{MEWT_VERSION}', prefix / 'bin' / 'mewt'
    if build:
        cargo = re.search(r'(\d+)\.(\d+)', subprocess.run(['cargo', '--version'], capture_output=True, text=True).stdout or '') \
            if shutil.which('cargo') else None
        if not cargo or (int(cargo[1]), int(cargo[2])) < RUST_FLOOR:
            raise Failure(f'building mewt needs cargo {".".join(map(str, RUST_FLOOR))} or newer (Ubuntu: sudo apt install cargo; '
                          'Arch: sudo pacman -S rust); nothing installed')
    with tempfile.TemporaryDirectory(prefix='cp-mewt-') as d:
        archive = Path(d) / 'mewt.tar'
        with urllib.request.urlopen(url, timeout=120) as r, open(archive, 'wb') as f:
            shutil.copyfileobj(r, f)
        have = hashlib.sha256(archive.read_bytes()).hexdigest()
        if have != digest:
            raise Failure(f'{url} has sha256 {have}, expected {digest}; nothing installed')
        with tarfile.open(archive) as tar:
            if build:
                try:
                    tar.extractall(Path(d) / 'src', filter='data')  # keeps every member, link included, inside the tree
                except tarfile.TarError as e:
                    raise Failure(f'{url}: unsafe archive member ({e}); nothing installed') from None
                tree = next((c.parent for c in sorted((Path(d) / 'src').glob('*/Cargo.lock'))), None)
                if tree is None:
                    raise Failure(f'{url} holds no Cargo.lock; nothing installed')
                print(f'building mewt {MEWT_VERSION} from source with cargo (several minutes)', flush=True)
                r = subprocess.run(['cargo', 'install', '--locked', '--path', str(tree), '--root', str(Path(d) / 'out')],
                                   capture_output=True, text=True)
                if r.returncode or not (Path(d) / 'out/bin/mewt').is_file():
                    raise Failure(f'cargo install exited {r.returncode}; nothing installed\n{tail(r.stdout + r.stderr)}')
                files = {'mewt': (Path(d) / 'out/bin/mewt').read_bytes()}
            else:
                members = [m for m in tar.getmembers() if m.isfile()]
                files = {Path(m.name).name: tar.extractfile(m).read() for m in members}
                modes = {Path(m.name).name: m.mode for m in members}
                if 'mewt' not in files:
                    raise Failure(f'{url} holds no mewt executable; nothing installed')
        dest.mkdir(parents=True, exist_ok=True)
        for name, data in files.items():
            target = dest / name
            target.write_bytes(data)
            target.chmod(0o755 if build or modes[name] & 0o111 else 0o644)
    link.parent.mkdir(parents=True, exist_ok=True)
    if link.is_symlink() or link.is_file():
        link.unlink()
    link.symlink_to(dest / 'mewt')
    print(f'installed mewt {MEWT_VERSION} in {dest}, linked as {link}')
    if Path(shutil.which('mewt') or '/').resolve() != (dest / 'mewt').resolve():
        print(f'WARNING {link.parent} is not first on PATH for mewt: add it before campaigns run, or they SKIP', flush=True)


def mewt_version():
    """The installed mewt version string, '' when it prints none, None when mewt is not on PATH."""
    if not shutil.which('mewt'):
        return None
    m = re.search(r'\d+\.\d+\.\d+', subprocess.run(['mewt', '--version'], capture_output=True, text=True).stdout)
    return m[0] if m else ''


def mewt(shard, *args):
    r = subprocess.run(['mewt', *args, '--log.color', 'off'], cwd=shard, capture_output=True, text=True)
    if r.returncode:
        raise Failure(f'mewt {" ".join(args)} exited {r.returncode} in {shard}\n{tail(r.stdout + r.stderr)}')
    return r.stdout


def tail(text, n=40):
    return '\n'.join(text.splitlines()[-n:])


def sync(root, shard):
    """Copy the working tree's tracked and untracked, not ignored files (outside SKIPPED) into shard."""
    for p in dict.fromkeys(git(root, 'ls-files', '-co', '--exclude-standard', '-z')):
        src, dst = root / p, shard / p
        if p.startswith(SKIPPED) or not (src.is_file() or src.is_symlink()):
            continue
        dst.parent.mkdir(parents=True, exist_ok=True)
        if dst.is_symlink() or dst.exists():
            dst.unlink()
        shutil.copy2(src, dst, follow_symlinks=False)


def measured(cmd, cwd, env=None):
    """(seconds, exit status, output, peak MB of its largest process) of one run of cmd."""
    with tempfile.NamedTemporaryFile('r', prefix='cp-mutation-rss-') as rss:
        probe = [sys.executable, '-c', 'import resource, subprocess, sys; c = subprocess.run(sys.argv[2:]).returncode; '
                 'open(sys.argv[1], "w").write(str(resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss)); sys.exit(c)', rss.name, *cmd]
        start = time.monotonic()
        p = subprocess.Popen(probe, cwd=cwd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, start_new_session=True)
        try:
            output = p.communicate()[0]
        except BaseException:  # a stopped campaign takes the capped tester down with it
            os.killpg(p.pid, signal.SIGTERM)
            try:
                p.wait(30)
            except subprocess.TimeoutExpired:
                os.killpg(p.pid, signal.SIGKILL)
                p.wait()
            raise
        return time.monotonic() - start, p.returncode, output, int(rss.read() or 0) >> 10


def available_mb():
    """MemAvailable in MB (16384 without /proc/meminfo)."""
    m = re.search(r'^MemAvailable:\s+(\d+)', Path('/proc/meminfo').read_text(), re.M) if Path('/proc/meminfo').is_file() else None
    return int(m[1]) >> 10 if m else 16384


def tester_cap(tester):
    """The memory cap the tester runs under: its ensure(mb=...) or the default."""
    m = re.search(r'ensure\([^)]*mb=(\d+)', tester.read_text(errors='replace'))
    return int(m[1]) if m else _00_memory_cap.MAIN_MB


def capped(cmd, mb):
    """cmd in its own memory-capped scope."""
    return [sys.executable, str(HERE / '_00_memory_cap.py'), '--mb', str(mb), '--', *cmd]


def interrupt(procs):
    """Stop mewt as Ctrl-C does (it restores the target), killing what still runs after 120 s."""
    for p in procs:
        if p.poll() is None:
            try:
                os.killpg(p.pid, signal.SIGINT)
            except ProcessLookupError:
                pass
    end = time.monotonic() + 120
    for p in procs:
        try:
            p.wait(max(0, end - time.monotonic()))
        except subprocess.TimeoutExpired:
            os.killpg(p.pid, signal.SIGKILL)
            p.wait()


def stage(name, todo, cmd, timeout, deadline):
    """Run `mewt test` on each shard's ids (todo: shard -> ids) concurrently; False when the deadline stopped it. A shard
    that fails before the deadline raises Failure with its log."""
    queue, running = [s for s in todo if todo[s]], {}
    if time.monotonic() >= deadline:
        return False
    for s in queue:
        (s / f'{name}.ids').write_text(''.join(f'{i}\n' for i in todo[s]))
    try:
        while queue or running:
            while queue:
                s = queue.pop(0)
                log = open(s / f'{name}.log', 'w')
                running[s] = subprocess.Popen(['mewt', 'test', '--ids-file', f'{name}.ids', '--test.cmd', cmd, '--test.timeout',
                                               str(timeout), '--log.color', 'off'], cwd=s, stdout=log, stderr=subprocess.STDOUT,
                                              start_new_session=True), log
            for s, (p, log) in list(running.items()):
                if p.poll() is not None:
                    log.close()
                    del running[s]
                    if p.returncode:
                        raise Failure(f'{name}: mewt test exited {p.returncode} in shard {s.name}\n'
                                      + tail((s / f'{name}.log').read_text(errors='replace')))
            if running and time.monotonic() >= deadline:
                return False
            time.sleep(0.5)
        return True
    finally:
        interrupt([p for p, _ in running.values()])
        for _, log in running.values():
            log.close()


def outcomes(shards):
    """{id: (status, mutant)}: each mutant's latest outcome over the shards (any shard may test any mutant)."""
    res = {}
    for s in shards:
        for r in json.loads(mewt(s, 'results', '--all', '--format', 'json'))['results']:
            o = r.get('outcome')
            when = o and re.fullmatch(r'(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d)(?:\.(\d+))?Z', o['time'])
            if o and not when:
                raise Failure(f'mewt outcome time {o["time"]!r} is not UTC ISO 8601')
            when = when and (when[1], (when[2] or '').ljust(12, '0'))  # fractions of varying length compare as numbers
            if o and (r['mutant']['id'] not in res or when > res[r['mutant']['id']][2]):
                res[r['mutant']['id']] = o['status'], r['mutant'], when
    return {i: v[:2] for i, v in res.items()}


def unchanged(header, shards):
    """Raise Failure when mewt left a mutant in a shard's copy of the header."""
    bad = [s.name for s in shards if (s / header).read_bytes() != (ROOT / header).read_bytes()]
    if bad:
        raise Failure(f'mewt left a mutated {header} in shard(s) {", ".join(bad)}; the outcomes after it are unreliable')


def severity_map(shard):
    text = mewt(shard, 'print', 'mutations')  # in a shard: mewt creates its database in the working directory
    cpp = text.split('Available mutations for cpp:', 1)[-1].split('Available mutations for', 1)[0]
    return {m[1]: m[2].lower() for m in re.finditer(r'\[(\w+)\].*\(Severity: (\w+)\)', cpp)}


def scope(header, data, now, whole):
    """(base blob or None, changed lines or None): lines changed since the recorded blob, None for the whole file."""
    found = record(ROOT, header)
    if whole or found is None:
        return None, None
    have = found[0]
    if have == now:  # current: no git lookup, the blob may be uncommitted
        if found[1] == 0:
            return have, []
        print(f'NOTE the record of blob {have[:12]} leaves mutants untested: whole-file campaign', flush=True)
        return None, None
    try:
        old = git(ROOT, 'cat-file', 'blob', have, raw=True)
    except Failure:
        print(f'NOTE blob {have[:12]} of the record is not in this repository: whole-file campaign', flush=True)
        return None, None
    return have, sorted(changed_lines(old, data))


def stop(*_):
    """SIGTERM or SIGHUP: stop as Ctrl-C does, once (a group signal also reaches us through the supervisor)."""
    for sig in (signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, signal.SIG_IGN)
    raise KeyboardInterrupt



def campaign(a):
    header = library_path(a.header)
    if not HEADER.fullmatch(header) or not (ROOT / header).is_file():
        raise Failure(f'{a.header}: not a library header (0N-<folder>/<NN-name>.hpp, aggregates excluded)')
    folder, stem = header.split('/')[0], Path(header).stem
    tester = f'96-Local Testing/{folder}/{stem}_tester.py'
    if not (ROOT / tester).is_file():
        raise Failure(f'{tester} does not exist: a header without its tester cannot be mutation-tested')
    version = mewt_version()
    if version != MEWT_VERSION:
        found = 'mewt not found' if version is None else f'mewt {version or "of unknown version"} found'
        print(f"SKIP mutation {header}: {found}, needs {MEWT_VERSION}; python3 '96-Local Testing/04-mutation.py' install")
        return 0
    os.environ['CP_TEST_PREFLIGHT'] = '0'  # a campaign is no tester: no quick pre-flight rerun of it under CP_TEST_MODE
    _00_memory_cap.ensure(force=True)  # the wrapper and mewt; every test runs in its own capped scope
    for sig in (signal.SIGTERM, signal.SIGHUP):  # a killed background run stops its shards as Ctrl-C does
        if signal.getsignal(sig) is not signal.SIG_IGN:
            signal.signal(sig, stop)
    data, now = (ROOT / header).read_bytes(), current(ROOT, header)
    work = Path(a.workdir).resolve() if a.workdir else None
    if work and work.is_relative_to(ROOT):
        raise Failure(f'{work} lies in the checkout, which every shard copies: choose a workdir outside it')
    meta_path = work and work / 'campaign.json'
    prior = json.loads(meta_path.read_text()) if meta_path and meta_path.is_file() else None
    if prior and (prior['header'], prior['blob']) == (header, now) and not a.whole:
        base, lines = prior['spec']['base'], None  # resume: a campaign keeps its scope once its record is current
    else:
        base, lines = scope(header, data, now, a.whole)
        if base == now:
            print(f'PASS {header}: the record already names blob {now[:12]}; --whole reruns the whole file')
            return 0
    severity = a.severity or ('high,medium' if base is None and CORE_FULL.fullmatch(header) else None)
    sample = a.sample if base is None else 0
    spec = dict(severity=severity, mutations=a.mutations, base=base, seed=a.seed, sample=sample)
    work = work or Path(tempfile.mkdtemp(prefix=f'cp-mutation-{stem}-'))
    meta_path = work / 'campaign.json'
    if work.is_dir() and any(f.name != '.lock' for f in work.iterdir()) and not meta_path.is_file():
        raise Failure(f'{work} is not empty and holds no campaign.json')
    work.mkdir(parents=True, exist_ok=True)
    lock = open(work / '.lock', 'w')
    try:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        raise Failure(f'another run uses {work}') from None
    if meta_path.is_file():
        meta = json.loads(meta_path.read_text())
        if (meta['header'], meta['blob'], meta['spec']) != (header, now, spec):
            raise Failure(f'{work} holds a campaign on {meta["header"]} blob {meta["blob"][:12]} sample {meta["spec"]}, not {header} '
                          f'blob {now[:12]} sample {spec}: start a new --workdir')
    else:
        meta = None
    where = os.environ.get('CP_MUTATION_LOCK') or Path(os.environ.get('XDG_RUNTIME_DIR') or tempfile.gettempdir()) / 'cp-mutation.lock'
    try:
        machine = open(where, 'w')
    except OSError as e:
        raise Failure(f'cannot open the campaign lock {where}: {e.strerror}') from None
    try:
        fcntl.flock(machine, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:  # concurrent campaigns would share memory and CPU: false catches by cap or timeout
        print(f'WAIT another campaign holds {machine.name}; this one starts when it ends', flush=True)
        fcntl.flock(machine, fcntl.LOCK_EX)
    if current(ROOT, header) != now:
        raise Failure(f'{header} changed while this run waited for the campaign lock: run it again')
    budget = a.memory_mb or available_mb() - 2048
    own = tester_cap(ROOT / tester)
    shards = [work / f's{k}' for k in range(meta['jobs'] if meta else a.jobs)]
    shards[0].mkdir(parents=True, exist_ok=True)
    for s in shards:
        if s.is_dir():
            sync(ROOT, s)
    print(f'MUTATION {header} blob {now[:12]} tester {tester} (cap {own} MB) workdir {work}; memory budget {budget} MB', flush=True)
    two = '--configuration' in subprocess.run([sys.executable, tester, '--help'], cwd=shards[0], capture_output=True, text=True).stdout
    run = lambda mode, *rest: [sys.executable, tester, '--mode', mode, '--seed', str(a.seed), *rest]
    stages = ([('quick', run('quick', '--configuration', 'optimized')), ('full-optimized', run('full', '--configuration', 'optimized')),
               ('full-rest', run('full', '--configuration', 'checked,ASan-UBSan'))] if two else [('quick', run('quick')), ('full', run('full'))])
    if meta is None:
        (shards[0] / 'mewt.toml').write_text(f'[targets]\ninclude = [{json.dumps(header)}]\n\n[test]\n'
                                             f'cmd = {json.dumps(shlex.join(stages[0][1]))}\ntimeout = 600\n')
        mewt(shards[0], 'mutate')
        listing = lambda *f: [(int(m[1]), range(int(m[2]), int(m[3] or m[2]) + 1)) for m in re.finditer(
            r'^\s*\[\w+ (\d+)\] Lines? (\d+)(?:-(\d+))?:', mewt(shards[0], 'print', 'mutants', *f), re.M)]
        pick = (['--severity', severity] if severity else []) + (['--mutation-types', a.mutations] if a.mutations else [])
        keep = set(lines) if lines is not None else None
        generated = len(mewt(shards[0], 'print', 'mutants', '--format', 'ids').split())
        found = listing()
        if len(found) != generated:
            raise Failure(f'parsed {len(found)} of the {generated} mutants mewt lists: the `print mutants` format changed')
        chosen = sorted(i for i, span in listing(*pick) if keep is None or keep.intersection(span))
        meta = dict(header=header, blob=now, spec=spec, lines=None if keep is None else len(keep), seed=a.seed, jobs=len(shards),
                    generated=generated, candidates=len(chosen), stages={}, unconfirmed={},
                    selected=sorted(random.Random(a.seed).sample(chosen, sample)) if 0 < sample < len(chosen) else chosen)
        for s in shards[1:]:
            shutil.copytree(shards[0], s, symlinks=True)
        meta_path.write_text(json.dumps(meta) + '\n')
    selected = set(meta['selected'])
    deadline = time.monotonic() + 60 * a.budget
    order = random.Random(a.seed).sample(sorted(selected), len(selected))
    complete, stopped = True, None
    for name, cmd in stages:
        seen = outcomes(shards)
        pending = [i for i in order if i not in seen or seen[i][0] not in CAUGHT]
        if not pending and name not in meta['unconfirmed'].values():
            continue
        if time.monotonic() >= deadline:
            complete, stopped = False, name
            break
        seconds, code, output, peak = measured(capped(cmd, own), shards[0], dict(os.environ, CCACHE_RECACHE='1'))  # a mutant compiles, so its limits must cover a compile
        if code:
            raise Failure(f'{name} baseline failed (exit {code}): {shlex.join(cmd)}\n{tail(output)}')
        timeout = a.timeout or max(60, math.ceil(3 * seconds))
        mb = min(own, max(1024, 3 * peak))  # each test in its own scope: a runaway mutant dies alone
        width = max(1, min(len(shards), budget // mb))
        start = time.monotonic()
        for part in ('', '-alone'):
            if part:  # a timeout among concurrent tests may be a slow survivor: it counts once it recurs with the CPU to itself
                seen = outcomes(shards)
                ids = [i for i in order if meta['unconfirmed'].get(str(i)) == name and seen[i][0] == 'Timeout']
                meta['unconfirmed'] = {k: v for k, v in meta['unconfirmed'].items() if v != name or int(k) in ids}
            builds = (1 if name != 'full-rest' else 2) if two else 3  # a test's builds run at once
            ids, w = (ids, max(1, min(width, len(ids)) // builds)) if part else (pending, width)
            if not ids:
                continue
            print(f'{name}{part}: {len(ids)} mutants on {w} of {len(shards)} shards; baseline {seconds:.1f} s, peak {peak} MB; per mutant '
                  f'timeout {timeout} s, cap {mb} MB; about {math.ceil(len(ids) * seconds / w / 60)} min, budget left '
                  f'{(deadline - time.monotonic()) / 60:.0f} min', flush=True)
            complete = stage(name + part, {s: ids[k::w] for k, s in enumerate(shards[:w])}, shlex.join(capped(cmd, mb)), timeout, deadline)
            unchanged(header, shards)
            kills = (_00_memory_cap.cgroup_stats() or (0, 0))[1] if os.environ.get('CP_MEMORY_CAPPED', 'none') != 'none' else 0
            if kills:
                raise Failure(f'{name}: the campaign cap killed {kills} process(es) outside the per-mutant scopes; outcomes are unreliable')
            seen = outcomes(shards)
            if part and complete:
                meta['unconfirmed'] = {k: v for k, v in meta['unconfirmed'].items() if v != name}
            elif not part:
                meta['unconfirmed'] |= {str(i): name for i in ids if seen.get(i, ('',))[0] == 'Timeout'}
            meta_path.write_text(json.dumps(meta) + '\n')
            if not complete:
                stopped = name + part
                break
        meta['stages'][name] = meta['stages'].get(name, 0) + round(time.monotonic() - start)
        meta_path.write_text(json.dumps(meta) + '\n')
        if not complete:
            break
    report(meta, {i: v for i, v in outcomes(shards).items() if i in selected}, stopped, work)
    if complete:
        git(ROOT, 'hash-object', '-w', '--', header)  # the recorded blob stays readable for the next diff scope
    return 0 if complete else 3


def report(meta, seen, stopped, work):
    complete = stopped is None
    counts = {}
    for status, _ in seen.values():
        counts[status] = counts.get(status, 0) + 1
    untested = len(meta['selected']) - len(seen) + counts.pop('Skipped', 0)
    timeouts, survived = counts.get('Timeout', 0), counts.get('Uncaught', 0)
    caught = counts.get('TestFail', 0) + timeouts
    other = {k: v for k, v in counts.items() if k not in (*CAUGHT, 'Uncaught')}
    spec = meta['spec']
    sample = ', '.join(x for x in (spec['severity'] and f'severity {spec["severity"]}', spec['mutations'] and f'types {spec["mutations"]}',
                                   len(meta['selected']) < meta['candidates'] and f'uniform sample of {meta["candidates"]}') if x)
    where = 'whole file' if spec['base'] is None else f'{meta["lines"]} lines changed since blob `{spec["base"][:12]}`'
    print(f'{"COMPLETE" if complete else "STOPPED by the budget"}: {len(seen)} of {len(meta["selected"])} sampled mutants tested '
          f'({meta["generated"]} generated), {caught} caught ({timeouts} by timeout), {survived} survived, {untested} untested'
          + (f', other outcomes {other}' if other else '') + f'; stage seconds {meta["stages"]}')
    rank = severity_map(work / 's0')
    survivors = sorted((m['line_offset'] + 1, i, m) for i, (status, m) in seen.items() if status == 'Uncaught')
    if survivors:
        cell = lambda t: '`' + re.sub(r'\s+', ' ', t).replace('|', '\\|').replace('`', "'")[:90] + '`'
        print('\n| Mutant | Line | Change |\n|---|---|---|')
        for line, i, m in survivors:
            print(f'| `{m["mutation_slug"]} {i}` ({rank.get(m["mutation_slug"], "?")}) | {line} | {cell(m["old_text"])} → {cell(m["new_text"])} |')
    if complete:
        print(f'\nRecord: mewt {MEWT_VERSION} on blob `{meta["blob"]}` ({date.today().isoformat()}, seed {meta["seed"]}, {where}'
              + (f', {sample}' if sample else '') + f'): {len(meta["selected"])} mutants, {caught} caught ({timeouts} by timeout), '
              f'{survived} survived, {untested} untested.')
    else:
        print(f'\nNo record: {untested} sampled mutants are untested and stage {stopped} did not finish; rerun with the same --workdir '
              'to finish the campaign.')
    print(f'Workdir {work} (rerun with --workdir to continue or to retest survivors; delete it once recorded)', flush=True)
    (work / 'report.json').write_text(json.dumps(dict(header=meta['header'], blob=meta['blob'], complete=complete, counts=counts,
                                                      untested=untested, survivors=[i for _, i, _ in survivors], stages=meta['stages'])) + '\n')


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0], formatter_class=argparse.RawDescriptionHelpFormatter, epilog=__doc__)
    sub = p.add_subparsers(dest='cmd', required=True)
    r = sub.add_parser('run', help='mutation campaign on one header')
    r.add_argument('header', help='library header (relative to the repository root or the working directory, or absolute)')
    r.add_argument('--budget', type=float, default=480, help='minutes for all stages (default 480); a stopped run resumes')
    r.add_argument('--jobs', type=int, default=min(6, max(1, (os.cpu_count() or 2) // 2)), help='shards, fixed when the campaign starts')
    r.add_argument('--memory-mb', type=int, default=0, help='memory budget of the concurrent tests (default: MemAvailable minus 2048)')
    r.add_argument('--seed', type=int, default=1, help='tester seed and mutant order')
    r.add_argument('--timeout', type=int, default=0, help='seconds per mutant (default: 3x the stage baseline, at least 60)')
    r.add_argument('--whole', action='store_true', help='every line, not only those changed since the recorded blob')
    r.add_argument('--severity', help=f'comma-separated subset of {",".join(SEVERITIES)} (default: high,medium on a Core full '
                   'whole-file campaign, else all)')
    r.add_argument('--mutations', help='comma-separated mewt mutation slugs (mewt print mutations)')
    r.add_argument('--sample', type=int, default=500, help='whole-file campaigns test a seeded uniform sample of at most this many '
                   'of the selected mutants (default 500; 0: all); a diff-scoped campaign tests all')
    r.add_argument('--workdir', help='campaign directory to create or resume (default: a new temporary directory, kept)')
    c = sub.add_parser('check', help='records match the current headers')
    c.add_argument('headers', nargs='*')
    c.add_argument('--existing', action='store_true', help='a header without a record passes (/prune, /restyle)')
    c.add_argument('--changed', nargs='?', const='master', metavar='BASE', help='every header changed against BASE (default master)')
    i = sub.add_parser('install', help=f'install mewt {MEWT_VERSION}')
    i.add_argument('--prefix', type=Path, default=Path.home() / '.local')
    i.add_argument('--source', action='store_true', help='build from the pinned source with cargo (the default on arm64)')
    a = p.parse_args(argv)
    try:
        if a.cmd == 'install':
            build = a.source or platform.machine() in ('aarch64', 'arm64')
            install(a.prefix, *((MEWT_SOURCE_URL, MEWT_SOURCE_SHA256) if build else (MEWT_URL, MEWT_SHA256)), build)
            return 0
        if a.cmd == 'check':
            headers = sorted({*map(library_path, a.headers), *(changed(ROOT, a.changed) if a.changed else ())})
            bad = [h for h in headers if not HEADER.fullmatch(h) or not (ROOT / h).is_file()]
            if bad:
                raise Failure(f'not library headers: {", ".join(bad)}')
            problems = stale(ROOT, headers, a.existing)
            for h, why in problems:
                print(f'FAIL {h}: {why}')
            print(f'{"FAIL" if problems else "PASS"}: {len(headers) - len(problems)} of {len(headers)} mutation records current')
            return int(bool(problems))
        if a.severity and not set(a.severity.split(',')) <= set(SEVERITIES):
            p.error(f'--severity takes a comma-separated subset of {",".join(SEVERITIES)}')
        if a.budget <= 0 or a.jobs < 1 or a.memory_mb < 0 or a.sample < 0:
            p.error('--budget and --jobs must be positive, --memory-mb and --sample nonnegative')
        return campaign(a)
    except Failure as e:
        print(f'FAIL {a.cmd}: {e}', file=sys.stderr, flush=True)
        return 1
    except KeyboardInterrupt:
        print(f'INTERRUPTED {a.cmd}', file=sys.stderr, flush=True)
        return 130


if __name__ == '__main__':
    raise SystemExit(main())
