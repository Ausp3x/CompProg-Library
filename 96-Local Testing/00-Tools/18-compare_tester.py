#!/usr/bin/env python3
"""Checks for 06-compare.py (A/B speed comparison of a header against its previous version) and 00-perf.hpp.

quick: the tool's functions on fixtures: row parsing, the include closure, side parsing, the export of a git ref, the
interleaved measurement on fake binaries (shell scripts whose output changes per pass) and the summary of a synthetic
log. full: quick plus end-to-end runs of 06-compare.py on a small rig built with the real compiler (CXX or g++) against
the real tree: base, work and a slowed side. stress: as full. Modes also come from CP_TEST_MODE.
Oracle: the expected rows, minima, geometric means and row lists, computed by hand from the fixtures; in the
end-to-end run the slowed side repeats its kernel eight times, so its row must be listed as losing in every build.
The end-to-end tests are skipped where the kernel refuses perf_event_open (kernel.perf_event_paranoid above 2).

Feature map:
a row line has six `|` fields, anything else fails -> test_parse_rows;
the 01-Core headers a rig includes, transitively, other includes ignored -> test_closure;
--side NAME=DIR[,MACRO...]: macros become -D options; a repeated or reserved name, a bad name and a missing directory
fail -> test_parse_sides;
the base side holds the headers of the git ref, not the working tree's -> test_export;
a row's value is its minimum-cycle pass with that pass's counters; a checksum that differs between sides or between
passes, a non-zero exit and differing row sets fail -> test_measure;
per side and build the geometric mean and counts; per compiler a row over the loss line in every build is `losing`, in
only some `layout`; the Comparison line (date, rig and header blobs, base, compiler versions, CPU, passes, rows, losing
count); --against another side -> test_summary;
Rows::net subtracts the counters of a shared prefix and never prints a value below the least one -> test_net;
the gate's native build is a group of its own, so a row slower only there is `losing` -> test_summary_native_group;
run builds every side in the three flag sets, writes the log, prints the summary; show prints it again; a slowed side
loses; --only, --filter; --gate adds the native build; a missing compiler fails with the tool's name -> test_e2e, test_e2e_missing_tool.
"""
import argparse
import importlib.util
import json
import math
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()
TOOL = Path(__file__).resolve().parents[1] / '06-compare.py'
ROOT = TOOL.parents[1]
spec = importlib.util.spec_from_file_location('compare', TOOL)
CMP = importlib.util.module_from_spec(spec)
spec.loader.exec_module(CMP)
MODE = 'full'

RIG = r'''#include "01-template.hpp"
#include "00-perf.hpp"
#ifdef SLOW
const int REPEAT = 8;
#else
const int REPEAT = 1;
#endif
int main(int argc, char **argv) {
    Rows rows(argc, argv); std::vector<ulng> a(4096); ulng s = 0;
    auto gen = [&] { for (ulng &x : a) { x = rows.rng(); } };
    rows.run("T", "xor", 4096, gen, [&] { for (int r = 0; r < REPEAT; ++r) { s = 0; for (ulng x : a) { s ^= x * 0x9e3779b97f4a7c15ULL >> 7; } __asm__("" : "+r"(s)); } }, [&] { return s; });
    rows.run("T", "sum", 4096, gen, [&] { s = 0; for (ulng x : a) { s += x >> 3; } }, [&] { return s; });
    rows.run("U", "sum", 4096, gen, [&] { s = 0; for (ulng x : a) { s += x >> 5; } }, [&] { return s; });
}
'''

NET_RIG = r'''#include "01-template.hpp"
#include "00-perf.hpp"
int main(int argc, char **argv) {
    Rows rows(argc, argv); std::vector<ulng> a(4096); ulng s = 0, t = 0;
    auto gen = [&] { for (ulng &x : a) { x = rows.rng(); } };
    auto first = [&] { t = 0; for (ulng x : a) { t ^= x * 0x9e3779b97f4a7c15ULL >> 7; } __asm__("" : "+r"(t)); };
    auto sum = [&] { s = 0; for (ulng x : a) { s += x * 0x100000001b3ULL >> 3; } __asm__("" : "+r"(s)); };
    rows.run("T", "sum", 4096, gen, sum, [&] { return s; });
    rows.run("T", "both", 4096, gen, [&] { first(); sum(); }, [&] { return s; });
    rows.net("T", "sum-net", 4096, gen, [&] { first(); sum(); }, first, [&] { return s; });
    rows.net("T", "nothing", 4096, gen, first, [&] { first(); sum(); }, [&] { return t; });
}
'''


class Compare(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix='cp-compare-test-'))
        self.addCleanup(shutil.rmtree, self.tmp, True)

    def test_parse_rows(self):
        rows = CMP.parse_rows('A|x|10.5|20|0.25|77\nA|y|1|2|0|9\n', 'here')
        self.assertEqual(rows, {'A|x': (10.5, 20.0, 0.25, '77'), 'A|y': (1.0, 2.0, 0.0, '9')})
        self.assertEqual(CMP.parse_rows('', 'here'), {})
        with self.assertRaisesRegex(CMP.Failure, 'here: not a row'):
            CMP.parse_rows('A|x|10.5|20|0.25|77\nwarning: something\n', 'here')

    def test_closure(self):
        core = self.tmp / '01-Core'
        core.mkdir()
        (core / '01-t.hpp').write_text('#pragma once\n')
        (core / '03-b.hpp').write_text('#include "01-t.hpp"\n#include <vector>\n')
        (core / '05-a.hpp').write_text('  #  include "03-b.hpp"\n#include "../elsewhere/09-z.hpp"\n// #include "07-no.hpp" in a comment line is not at the line start\n')
        (core / '07-no.hpp').write_text('')
        (core / '09-unused.hpp').write_text('')
        rig = self.tmp / 'rig.cpp'
        rig.write_text('#include "05-a.hpp"\n#include "00-perf.hpp"\n#include "../01-Core/03-b.hpp"\n')
        self.assertEqual(CMP.closure(rig, core), ['01-t.hpp', '03-b.hpp', '05-a.hpp'])

    def test_parse_sides(self):
        d = self.tmp / 'proto'
        d.mkdir()
        sides = CMP.parse_sides([f'p={d},A,B=2', f'q={d}'], self.tmp / 'base', self.tmp)
        self.assertEqual(sides, [('base', self.tmp / 'base', []), ('work', self.tmp / '01-Core', []), ('p', d.resolve(), ['-DA', '-DB=2']), ('q', d.resolve(), [])])
        for bad in (f'work={d}', f'p={d}', f'a b={d}', f'x={self.tmp}/missing', 'x'):
            with self.assertRaises(CMP.Failure, msg=bad):
                CMP.parse_sides([f'p={d}', bad] if bad.startswith('p=') else [bad], self.tmp / 'base', self.tmp)

    def test_export(self):
        git = lambda *a: subprocess.run(['git', '-c', 'user.name=t', '-c', 'user.email=t@t', '-c', 'commit.gpgsign=false', *a], cwd=self.tmp, check=True, capture_output=True)
        git('init', '-q')
        (self.tmp / '01-Core/Docs').mkdir(parents=True)
        (self.tmp / '01-Core/05-a.hpp').write_text('old\n')
        (self.tmp / '01-Core/00-index.md').write_text('index\n')
        (self.tmp / '01-Core/Docs/05-a.hpp').write_text('nested\n')
        git('add', '-A')
        git('commit', '-q', '-m', 'one')
        (self.tmp / '01-Core/05-a.hpp').write_text('new\n')
        (self.tmp / '01-Core/06-b.hpp').write_text('untracked\n')
        commit = CMP.export('HEAD', self.tmp / 'out', self.tmp)
        self.assertEqual(commit, CMP.git('rev-parse', 'HEAD', root=self.tmp).strip())
        self.assertEqual({p.name: p.read_text() for p in (self.tmp / 'out').iterdir()}, {'05-a.hpp': 'old\n'})
        with self.assertRaisesRegex(CMP.Failure, 'git '):
            CMP.export('no-such-ref', self.tmp / 'out2', self.tmp)

    def fake(self, name, passes, code=0):
        """A binary whose k-th run prints passes[k] (one row list per run) and exits with `code`."""
        path = self.tmp / name
        for k, text in enumerate(passes):
            (self.tmp / f'{name}.{k}').write_text(text)
        path.write_text(f'#!/bin/sh\nn=$(cat {shlex.quote(str(path))}.n 2>/dev/null || echo 0)\necho $((n + 1)) > {shlex.quote(str(path))}.n\n'
                        f'cat {shlex.quote(str(path))}.$n\nexit {code}\n')
        path.chmod(0o755)
        return path

    def test_measure(self):
        a = self.fake('a', ['T|x|10|100|1|5\nT|y|7|70|0|6\n', 'T|x|9|90|2|5\nT|y|8|80|0.5|6\n'])
        b = self.fake('b', ['T|x|20|100|1|5\nT|y|3|70|0|6\n', 'T|x|21|90|2|5\nT|y|2|80|0.5|6\n'])
        best = CMP.measure({('base', 'g scalar'): a, ('work', 'g scalar'): b}, 2, 0, '', 10)
        self.assertEqual(best, {('base', 'g scalar'): {'T|x': [9.0, 90.0, 2.0], 'T|y': [7.0, 70.0, 0.0]},
                                ('work', 'g scalar'): {'T|x': [20.0, 100.0, 1.0], 'T|y': [2.0, 80.0, 0.5]}})
        cases = {'between sides': (['T|x|10|100|1|5\n'], ['T|x|10|100|1|4\n'], 'checksum of T\\|x differs'),
                 'between passes': (['T|x|10|100|1|5\n', 'T|x|10|100|1|8\n'], ['T|x|10|100|1|5\n', 'T|x|10|100|1|5\n'], 'checksum of T\\|x differs'),
                 'row sets': (['T|x|10|100|1|5\n'], ['T|y|10|100|1|5\n'], 'different row sets'),
                 'no rows': ([''], [''], 'different row sets, or none')}
        for k, (name, (x, y, message)) in enumerate(cases.items()):
            with self.assertRaisesRegex(CMP.Failure, message, msg=name):
                CMP.measure({('base', 'g scalar'): self.fake(f'x{k}', x), ('work', 'g scalar'): self.fake(f'y{k}', y)}, len(x), 0, '', 10)
        with self.assertRaisesRegex(CMP.Failure, 'work g scalar pass 1: exit 3'):
            CMP.measure({('base', 'g scalar'): self.fake('ok', ['T|x|1|1|0|5\n']), ('work', 'g scalar'): self.fake('bad', ['T|x|1|1|0|5\n'], 3)}, 1, 0, '', 10)
        # the two builds of one side keep separate checksums: a build may legitimately differ only if the rig says so, never silently
        with self.assertRaisesRegex(CMP.Failure, 'checksum'):
            CMP.measure({('base', 'g scalar'): self.fake('p', ['T|x|1|1|0|5\n']), ('work', 'g scalar'): self.fake('q', ['T|x|1|1|0|6\n']),
                         ('base', 'g isa'): self.fake('r', ['T|x|1|1|0|5\n']), ('work', 'g isa'): self.fake('s', ['T|x|1|1|0|5\n'])}, 1, 0, '', 10)

    def log(self):
        row = lambda *c: [[x, 100.0, 0.0] for x in c]
        builds = ['g++-15 scalar', 'g++-15 isa', 'g++-15 layout', 'g++-14 scalar']
        names = ['T|a', 'T|b', 'T|c', 'T|d']
        base = {'T|a': row(10, 10, 10, 10), 'T|b': row(10, 10, 10, 10), 'T|c': row(10, 10, 10, 10), 'T|d': row(10, 10, 10, 10)}
        # a: slower everywhere (losing on both compilers); b: slower in one g++-15 build (layout); c: faster; d: slower only on g++-14
        work = {'T|a': row(11, 12, 10.4, 10.5), 'T|b': row(10, 10.5, 10.2, 10), 'T|c': row(5, 5, 5, 10), 'T|d': row(10, 10, 10.2, 12)}
        other = {'T|a': row(10, 10, 10, 10), 'T|b': row(10, 10, 10, 10), 'T|c': row(10, 10, 10, 10), 'T|d': row(10, 10, 10, 10)}
        split = lambda side: {b: {n: side[n][k] for n in names} for k, b in enumerate(builds)}
        return {'conditions': {'date': '2026-11-17', 'rig': '96-Local Testing/01-Core/05-x_compare.cpp', 'rig_blob': 'a' * 40, 'base': 'b' * 40,
                               'headers': {'01-template.hpp': 'c' * 40, '05-x.hpp': 'd' * 40}, 'compilers': {'g++-15': '15.3.0', 'g++-14': '14.4.1'},
                               'cpu': 'Test CPU', 'passes': 3, 'filter': '', 'core': 5, 'sides': {}},
                'rows': {'base': split(base), 'work': split(work), 'other': split(other)}}

    def test_summary(self):
        lines = CMP.summary(self.log(), 'base', 0.03, 40)
        gm = lambda *x: f'{math.exp(sum(map(math.log, x)) / len(x)):.3f}'
        self.assertEqual(lines[0], 'work against base')
        self.assertEqual(lines[1], f'  g++-15 scalar: 4 rows, geometric mean {gm(1.1, 1.0, 0.5, 1.0)}, 1 over 3% slower, 1 over 3% faster')
        self.assertEqual(lines[2], f'  g++-15 isa: 4 rows, geometric mean {gm(1.2, 1.05, 0.5, 1.0)}, 2 over 3% slower, 1 over 3% faster')
        self.assertEqual(lines[4], f'  g++-14 scalar: 4 rows, geometric mean {gm(1.05, 1.0, 1.0, 1.2)}, 2 over 3% slower, 0 over 3% faster')
        self.assertEqual(lines[5], '  g++-15 losing (1): T|a 1.10/1.20/1.04')
        self.assertEqual(lines[6], '  g++-15 layout (1): T|b 1.00/1.05/1.02')
        self.assertEqual(lines[7], '  g++-14 losing (2): T|d 1.20, T|a 1.05')
        self.assertEqual(lines[8], 'other against base')
        self.assertTrue(all('geometric mean 1.000, 0 over 3% slower, 0 over 3% faster' in x for x in lines[9:13]), lines[9:13])
        self.assertEqual(lines[13], 'Comparison: 2026-11-17, rig 96-Local Testing/01-Core/05-x_compare.cpp aaaaaaaaaaaa, base bbbbbbbbbbbb, headers '
                                    '01-template.hpp cccccccccccc 05-x.hpp dddddddddddd, g++-15 15.3.0, g++-14 14.4.1, CPU Test CPU, 3 passes, 4 rows, 3 losing over 3%.')
        self.assertEqual(len(lines), 14)
        top = CMP.summary(self.log(), 'base', 0.03, 1)
        self.assertEqual(top[7], '  g++-14 losing (2): T|d 1.20 ...')
        flipped = CMP.summary(self.log(), 'work', 0.03, 40)
        self.assertEqual(flipped[0], 'base against work')
        self.assertIn('g++-15 losing (1): T|c 2.00/2.00/2.00', flipped[5])
        self.assertEqual(CMP.summary(self.log(), 'base', 0.25, 40)[5], 'other against base', 'no row is 25% slower in every build')

    def test_summary_native_group(self):
        log = self.log()
        for side in log['rows'].values():
            side['g++-15 native'] = side.pop('g++-14 scalar')
        lines = CMP.summary(log, 'base', 0.03, 40)
        self.assertEqual(lines[5], '  g++-15 losing (1): T|a 1.10/1.20/1.04', 'the native build is no build of the g++-15 group')
        self.assertEqual(lines[7], '  g++-15 native losing (2): T|d 1.20, T|a 1.05')

    def run_tool(self, *args):
        return subprocess.run([sys.executable, str(TOOL), *args], capture_output=True, text=True, timeout=900)

    def rig(self):
        cxx = os.environ.get('CXX', 'g++')
        rig = self.tmp / 'small_compare.cpp'
        rig.write_text(RIG)
        probe = self.tmp / 'probe'
        subprocess.run([*shlex.split(cxx), '-std=gnu++20', '-O2', '-I', str(ROOT / '01-Core'), '-I', str(TOOL.parent), '-o', str(probe), str(rig)], check=True)
        r = subprocess.run([str(probe), 'no-such-row'], capture_output=True, text=True)
        if r.returncode == 2 and 'perf_event_open' in r.stderr:
            self.skipTest('the kernel refuses perf_event_open here')
        self.assertEqual((r.returncode, r.stdout), (0, ''), r.stderr)
        return cxx, rig

    def test_e2e(self):
        if MODE == 'quick':
            self.skipTest('full mode')
        cxx, rig = self.rig()
        out = self.tmp / 'log.json'
        r = self.run_tool('run', str(rig), '--compilers', cxx, '--side', f'slow={ROOT / "01-Core"},SLOW', '--passes', '2', '--core', '0', '--out', str(out))
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        log = json.loads(out.read_text())
        builds = [f'{cxx} {b}' for b in ('scalar', 'isa', 'layout')]
        self.assertEqual({s: list(b) for s, b in log['rows'].items()}, {s: builds for s in ('base', 'work', 'slow')})
        self.assertTrue(all(set(rows) == {'T|xor', 'T|sum', 'U|sum'} and all(c[0] > 0 and c[1] > 0 for c in rows.values())
                            for side in log['rows'].values() for rows in side.values()))
        c = log['conditions']
        self.assertEqual((c['base'], list(c['headers']), c['passes'], c['sides']['slow'][1]),
                         (CMP.git('rev-parse', 'HEAD').strip(), ['01-template.hpp'], 2, ['-DSLOW']))
        self.assertEqual(c['headers']['01-template.hpp'], CMP.git('hash-object', str(ROOT / '01-Core/01-template.hpp')).strip())
        report = lambda text: [x for x in text.splitlines() if not x.startswith('MEMORY ')]  # the tool's own cap report
        lines = report(r.stdout)
        slow = lines[lines.index('slow against base'):]
        self.assertTrue(any(x.startswith(f'  {cxx} losing (') and 'T|xor' in x for x in slow), r.stdout)
        ratios = [log['rows']['slow'][b]['T|xor'][1] / log['rows']['base'][b]['T|xor'][1] for b in builds]
        self.assertTrue(all(6 < x < 10 for x in ratios), f'eight repetitions, instructions per unit: {ratios}')
        self.assertTrue(any(x.startswith('Comparison: ') and x.endswith('.') and '3 rows' in x for x in lines), r.stdout)
        self.assertEqual(lines[-1], f'log: {out}')
        shown = self.run_tool('show', str(out))
        self.assertEqual((shown.returncode, report(shown.stdout)), (0, lines[lines.index('work against base'):-1]))
        r = self.run_tool('run', str(rig), '--compilers', cxx, '--only', 'base,work', '--filter', 'U sum,T xor', '--gate', '--passes', '1', '--core', '0', '--out', str(out))
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        log = json.loads(out.read_text())
        self.assertEqual((list(log['rows']), set(log['rows']['work'][builds[0]])), (['base', 'work'], {'T|xor', 'U|sum'}))
        self.assertEqual(list(log['rows']['work']), builds + [f'{cxx} native'], '--gate adds the native build on the deciding compiler')

    def test_net(self):
        if MODE == 'quick':
            self.skipTest('full mode')
        cxx, _ = self.rig()
        rig, probe = self.tmp / 'net_compare.cpp', self.tmp / 'net'
        rig.write_text(NET_RIG)
        subprocess.run([*shlex.split(cxx), '-std=gnu++20', '-O2', '-I', str(ROOT / '01-Core'), '-I', str(TOOL.parent), '-o', str(probe), str(rig)], check=True)
        r = subprocess.run([str(probe)], capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stderr)
        rows = CMP.parse_rows(r.stdout, 'net rig')
        self.assertEqual(set(rows), {'T|sum', 'T|both', 'T|sum-net', 'T|nothing'})
        self.assertGreater(rows['T|both'][1], 1.8 * rows['T|sum'][1], 'the unsubtracted row holds both kernels')
        self.assertLess(abs(rows['T|sum-net'][1] / rows['T|sum'][1] - 1), 0.1, 'instructions per unit of the kernel alone')
        self.assertEqual(rows['T|nothing'][:3], (0.001, 0.001, 0.0), 'a difference below zero is printed as the least value')

    def test_e2e_missing_tool(self):
        if MODE == 'quick':
            self.skipTest('full mode')
        rig = self.tmp / 'small_compare.cpp'
        rig.write_text(RIG)
        r = self.run_tool('run', str(rig), '--compilers', 'g++-no-such-version', '--out', str(self.tmp / 'log.json'))
        self.assertEqual(r.returncode, 1)
        self.assertIn('FAIL missing tool: g++-no-such-version', r.stdout)
        self.assertFalse((self.tmp / 'log.json').exists())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')), help='unused: no randomness')
    MODE = parser.parse_args().mode
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(Compare)
    print(f'compare checks: mode={MODE} tests={suite.countTestCases()}', flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
