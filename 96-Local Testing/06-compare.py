#!/usr/bin/env python3
"""A/B speed comparison of a Core full header against its previous version (05-testing.md, Benchmarks).

run <rig.cpp> builds the rig once per side and build and times the binaries interleaved on one pinned core with hardware
counters. A side is a set of 01-Core headers first on the include path: `base` is 01-Core at --base (a git ref, default
HEAD), `work` the working tree, and each --side NAME=DIR[,MACRO...] a directory of headers (a prototype) with macros.
The builds are the scalar, ISA and layout flag sets on the deciding compiler, the first of --compilers, and with --gate
an -O2 -march=native build on it and the scalar set on each further compiler. A rig includes the header under test and `00-perf.hpp`, prints one line
`type|row|cycles|instructions|misses|checksum` per row and takes a comma-separated row filter. Each pass runs every
binary once; a row's value is its minimum cycles over the passes. A checksum that differs between passes or sides fails
the run. The log goes to <rig stem>.json beside the rig (git-ignored).
show <log.json> prints the summary of a recorded run again.

The summary gives, per side against --against and per build, the geometric mean of the ratios and the rows more than
--loss slower: `losing` in every build of a compiler, `layout` in only some of them. The `Comparison:` line belongs in
the header's evidence document; /finish compares its compiler versions and header blobs with the current ones.
"""
import argparse
import datetime
import json
import math
import os
from pathlib import Path
import platform
import re
import shlex
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _00_memory_cap
from _00_cpp_test_runner import Builder, ready

ROOT = Path(__file__).resolve().parents[1]
CORE = '01-Core'
FLAGS = {'scalar': ['-O2'], 'isa': ['-O2', '-mavx2', '-mbmi2', '-mfma'], 'layout': ['-O2', '-falign-functions=64', '-falign-loops=32']}
GATE = {'native': ['-O2', '-march=native']}  # with --gate on the deciding compiler, as AtCoder compiles; a group of its own in the summary
COMPILERS = 'g++-15,g++-14,g++'  # the deciding compiler first (03-cpp.md, Toolchain and Judges), then the floor and the local one
BUILD_MB = 2600  # measured: one -O2 build of the modint rig peaks near 2.5 GB


class Failure(RuntimeError):
    pass


def git(*args, root=ROOT):
    r = subprocess.run(['git', *args], cwd=root, capture_output=True, text=True)
    if r.returncode:
        raise Failure(f'git {shlex.join(args)}: {r.stderr.strip()}')
    return r.stdout


def export(ref, directory, root=ROOT):
    """Write the 01-Core headers of `ref` into `directory`; returns the commit."""
    directory.mkdir(parents=True)
    for name in git('ls-tree', '--name-only', ref, CORE + '/', root=root).split('\n'):
        if name.endswith('.hpp'):
            (directory / Path(name).name).write_text(git('show', f'{ref}:{name}', root=root))
    return git('rev-parse', ref, root=root).strip()


def closure(rig, core):
    """The 01-Core headers the rig includes, transitively, by quoted name."""
    seen, todo = set(), [rig]
    while todo:
        for name in re.findall(r'^\s*#\s*include\s*"([^"]+)"', todo.pop().read_text(errors='replace'), re.M):
            path = core / Path(name).name
            if path.is_file() and path.name not in seen:
                seen.add(path.name)
                todo.append(path)
    return sorted(seen)


def parse_sides(specs, base_dir, root=ROOT):
    sides = [('base', base_dir, []), ('work', root / CORE, [])]
    for spec in specs:
        name, _, rest = spec.partition('=')
        directory, *macros = rest.split(',')
        if not re.fullmatch(r'\w+', name) or name in {s[0] for s in sides} or not directory or not Path(directory).is_dir():
            raise Failure(f'--side {spec}: expected NAME=DIR[,MACRO...] with a new name and an existing directory')
        sides.append((name, Path(directory).resolve(), ['-D' + m for m in macros]))
    return sides


def parse_rows(text, where):
    rows = {}
    for line in text.splitlines():
        parts = line.split('|')
        if len(parts) != 6:
            raise Failure(f'{where}: not a row: {line[:120]!r}')
        rows[parts[0] + '|' + parts[1]] = (float(parts[2]), float(parts[3]), float(parts[4]), parts[5])
    return rows


def measure(binaries, passes, core, row_filter, timeout):
    """{(side, build): {row: [cycles, instructions, misses]}} at the minimum-cycle pass; binaries run interleaved."""
    best, sums = {k: {} for k in binaries}, {}
    for p in range(passes):
        for key, binary in binaries.items():
            r = subprocess.run(['taskset', '-c', str(core), str(binary), row_filter], capture_output=True, text=True, timeout=timeout)
            if r.returncode:
                raise Failure(f'{key[0]} {key[1]} pass {p + 1}: exit {r.returncode}\n{r.stderr[-600:]}')
            for row, (c, i, m, s) in parse_rows(r.stdout, f'{key[0]} {key[1]}').items():
                if sums.setdefault((key[1], row), s) != s:
                    raise Failure(f'checksum of {row} differs: {key[0]} {key[1]} pass {p + 1} printed {s}, another run {sums[(key[1], row)]}')
                if row not in best[key] or c < best[key][row][0]:
                    best[key][row] = [c, i, m]
    rows = {frozenset(v) for v in best.values()}
    if len(rows) != 1 or not next(iter(rows)):
        raise Failure('the binaries printed different row sets, or none (check --filter)')
    return best


def gm(values):
    return math.exp(sum(map(math.log, values)) / len(values))


def summary(log, against, loss, top):
    """Lines: per side and build the geometric mean and counts, then the losing and layout-sensitive rows per compiler."""
    data = {(s, b): rows for s, builds in log['rows'].items() for b, rows in builds.items()}
    builds = list(log['rows'][against])
    lines, over = [], 0
    for side in log['rows']:
        if side == against:
            continue
        lines.append(f'{side} against {against}')
        ratios = {b: {r: data[(side, b)][r][0] / v[0] for r, v in data[(against, b)].items()} for b in builds}
        for b in builds:
            x = ratios[b].values()
            lines.append(f'  {b}: {len(x)} rows, geometric mean {gm(x):.3f}, {sum(v > 1 + loss for v in x)} over {loss:.0%} slower, '
                         f'{sum(v < 1 - loss for v in x)} over {loss:.0%} faster')
        group = lambda b: b if b.split(' ')[-1] in GATE else b.split(' ')[0]
        for compiler in dict.fromkeys(group(b) for b in builds):
            mine = [b for b in builds if group(b) == compiler]
            worst = {r: [ratios[b][r] for b in mine] for r in ratios[mine[0]]}
            losing = sorted(((min(v), r) for r, v in worst.items() if min(v) > 1 + loss), reverse=True)
            layout = sorted(((max(v), r) for r, v in worst.items() if max(v) > 1 + loss >= min(v)), reverse=True)
            over += len(losing)
            for label, found in (('losing', losing), ('layout', layout)):
                if found:
                    lines.append(f'  {compiler} {label} ({len(found)}): ' + ', '.join(
                        f'{r} {"/".join(f"{x:.2f}" for x in worst[r])}' for _, r in found[:top]) + (' ...' if len(found) > top else ''))
    c = log['conditions']
    lines.append(f'Comparison: {c["date"]}, rig {c["rig"]} {c["rig_blob"][:12]}, base {c["base"][:12]}, headers '
                 + ' '.join(f'{n} {b[:12]}' for n, b in c['headers'].items()) + ', '
                 + ', '.join(f'{k} {v}' for k, v in c['compilers'].items()) + f', CPU {c["cpu"]}, {c["passes"]} passes, '
                 f'{len(data[(against, builds[0])])} rows, {over} losing over {loss:.0%}.')
    return lines


def cpu_model():
    try:
        return next(l.split(':', 1)[1].strip() for l in Path('/proc/cpuinfo').read_text().splitlines() if l.startswith('model name'))
    except (OSError, StopIteration):
        return platform.processor() or 'unknown'


def run(a):
    rig = a.rig.resolve()
    compilers = [c for c in a.compilers.split(',') if c]
    missing = [c for c in compilers if shutil.which(shlex.split(c)[0]) is None]
    if missing or shutil.which('taskset') is None:
        raise Failure('missing tool: ' + ', '.join(missing + ([] if shutil.which('taskset') else ['taskset'])) + ' (/setup-check prints the install commands)')
    if not a.gate:
        compilers = compilers[:1]
    with tempfile.TemporaryDirectory(prefix='cp-compare-') as directory:
        tmp = Path(directory)
        commit = export(a.base, tmp / 'base')
        sides = parse_sides(a.side, tmp / 'base')
        if a.only:
            sides = [s for s in sides if s[0] in a.only.split(',')]
        binaries, versions = {}, {}
        for k, compiler in enumerate(compilers):
            cxx = shlex.split(compiler)
            versions[compiler] = subprocess.run([*cxx, '-dumpfullversion'], capture_output=True, text=True).stdout.strip()
            builder, jobs = Builder(cxx, tmp, failure=Failure), []
            for build, flags in (FLAGS | GATE if a.gate else FLAGS).items():
                if k and build != 'scalar':
                    continue
                for name, inc, macros in sides:
                    out = tmp / f'{name}-{k}-{build}'
                    binaries[(name, f'{compiler} {build}')] = out
                    jobs.append((out.name, f'build {name} {compiler} {build}', ['-std=gnu++20', *flags, *macros, '-I', str(inc), '-I', str(ROOT / CORE),
                                                                                '-I', str(Path(__file__).resolve().parent)], [rig], out, a.timeout, BUILD_MB))
            builder.build_all(jobs)
            for job in jobs:
                ready(job[4])
            print(f'built {len(jobs)} binaries with {compiler} {versions[compiler]}', flush=True)
        best = measure(binaries, a.passes, a.core, a.filter, a.timeout)
    rows = {}
    for (side, build), found in best.items():
        rows.setdefault(side, {})[build] = found
    rel = lambda p: str(p.relative_to(ROOT)) if p.is_relative_to(ROOT) else str(p)
    log = {'conditions': {'date': datetime.date.today().isoformat(), 'rig': rel(rig), 'rig_blob': git('hash-object', str(rig)).strip(), 'base': commit,
                          'headers': {n: git('hash-object', str(ROOT / CORE / n)).strip() for n in closure(rig, ROOT / CORE)},
                          'compilers': versions, 'cpu': cpu_model(), 'passes': a.passes, 'filter': a.filter, 'core': a.core,
                          'sides': {n: [rel(d) if n != 'base' else a.base, m] for n, d, m in sides}}, 'rows': rows}
    out = a.out or rig.with_suffix('.json')
    out.write_text(json.dumps(log) + '\n')
    print('\n'.join(summary(log, a.against, a.loss, a.top)))
    print(f'log: {rel(out)}')


def options():
    """The command line: run or show."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    r = sub.add_parser('run')
    r.add_argument('rig', type=Path, help='the rig source, <NN-name>_compare.cpp beside the header\'s tester')
    r.add_argument('--base', default='HEAD', help='git ref of the previous header (default HEAD)')
    r.add_argument('--side', action='append', default=[], metavar='NAME=DIR[,MACRO...]', help='a further side: headers in DIR first, with -DMACRO')
    r.add_argument('--only', help='comma-separated sides to build and run (default: all)')
    r.add_argument('--compilers', default=os.environ.get('CP_COMPARE_COMPILERS', COMPILERS), help=f'deciding compiler first (default {COMPILERS})')
    r.add_argument('--gate', action='store_true', help='also a -march=native build and the scalar build on each further compiler (the finished header)')
    r.add_argument('--filter', default='', help='comma-separated substrings of "type row"')
    r.add_argument('--passes', type=int, default=3)
    r.add_argument('--core', type=int, default=min(5, (os.cpu_count() or 1) - 1), help='the CPU the binaries are pinned to')
    r.add_argument('--timeout', type=int, default=1800, help='seconds per build and per binary run')
    r.add_argument('--out', type=Path, help='log file (default: <rig stem>.json beside the rig)')
    s = sub.add_parser('show')
    s.add_argument('log', type=Path)
    for p in (r, s):
        p.add_argument('--against', default='base', help='the side the others are divided by (default base)')
        p.add_argument('--loss', type=float, default=0.03, help='a row this much slower is listed (default 0.03)')
        p.add_argument('--top', type=int, default=40, help='rows listed per group')
    return parser.parse_args()


def variant():
    """What a run's peak depends on, for its .run-stats.json key: the rig, the compilers and --gate (Builder runs at most
    three builds at once, whatever the sides)."""
    a = options()
    rig = a.rig.resolve()
    return f"{rig.relative_to(ROOT) if rig.is_relative_to(ROOT) else rig} --compilers {a.compilers}{' --gate' if a.gate else ''}"


def main():
    a = options()
    try:
        if a.command == 'run':
            run(a)
        else:
            print('\n'.join(summary(json.loads(a.log.read_text()), a.against, a.loss, a.top)))
    except Failure as error:
        print(f'FAIL {error}')
        return 1
    return 0


if __name__ == '__main__':
    _00_memory_cap.ensure(mb=3 * BUILD_MB + 1024, need=None if sys.argv[1:2] == ['run'] else 0, variant=variant)  # three builds at once (Builder), each near 2.5 GB; only run builds
    raise SystemExit(main())
