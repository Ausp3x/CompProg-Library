#!/usr/bin/env python3
"""Network-free checks for 07-judge.py, the Library Checker tool of /speed-sweep.

The API is a fake `fetch` serving fixed JSON (four fastest users, two of them the same code under a header comment and
other whitespace, a pending submission that finishes on the second fetch, a user's WA/AC history); the problem repository
is a fixture `sample/aplusb` (two .in examples, a seeded generator using params.h, a reference solution, a token checker)
plus `sample/tiny` (one example, 0.2 s limit). Oracles: generator inputs and params.h recomputed in Python, the
statuses each fixture solution must get, hand-computed similarities and table cells, fetch counters for caching.
quick: similarity, dedupe, caching, offline, mine, compare, problems and one trusted reproduction.
full: also WA/RE/TLE (wall and CPU limit), min-of-runs, regeneration stamps, the fetched-source refusal and the bwrap
sandbox (SKIP without bwrap). stress: full plus --rounds x 50 seeded random similarity cases.
"""
import argparse
import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import random
import shutil
import sys
import tempfile
import unittest
import urllib.parse
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

spec = importlib.util.spec_from_file_location('judge', Path(__file__).with_name('07-judge.py'))
J = importlib.util.module_from_spec(spec)
spec.loader.exec_module(J)
MODE, SEED, ROUNDS = 'full', 42, 6
QUICK = {'test_similarity', 'test_fastest_dedupe', 'test_submission_cache', 'test_offline', 'test_mine', 'test_compare',
         'test_problems', 'test_reproduce_ok'}

A = '#include <cstdio>\nint main() {\n    long long a, b; // read\n    scanf("%lld%lld", &a, &b);\n    printf("%lld //x\\n", a + b);\n}\n'
A_COPY = '/* Copied by Rohan_Kapri\n   2026 */\n#include <cstdio>\nint main()   {\n\tlong long a,b;\n  scanf("%lld%lld",&a,&b);\n' \
         '    printf("%lld //x\\n", a+b);\n}\n'
B = '#include <bits/stdc++.h>\nint main() {\n    std::int64_t a, b;\n    std::cin >> a >> b;\n    std::cout << a + b << "\\n";\n}\n'
PY = 'a, b = map(int, input().split())\nprint(a + b // 1)\n'
LIST = [dict(id=12, user_name='Rohan_Kapri', lang='cpp', time=0.009, memory=3 << 20),
        dict(id=11, lang='cpp', time=0.009, memory=2 << 20),
        dict(id=13, user_name='Leader', lang='cpp', time=0.020, memory=4 << 20),
        dict(id=14, user_name='Snake', lang='python3', time=0.500, memory=9 << 20)]
SOURCES = {12: A_COPY, 11: A, 13: B, 14: PY, 21: B, 22: B, 30: B}
CASES = {12: {'example_00': 0.001, 'random_00': 0.004, 'random_01': 0.006},
         11: {'example_00': 0.001, 'random_00': 0.004, 'random_01': 0.006},
         13: {'example_00': 0.002, 'random_00': 0.002, 'random_01': 0.010, 'judge_only_00': 0.003},
         14: {'example_00': 0.1, 'random_00': 0.5, 'random_01': 0.4},
         21: {'example_00': 0.002, 'random_00': 0.008, 'random_01': 0.012}, 22: {}, 30: {}}
MINE = [dict(id=22, user_name='Ausp3x', lang='cpp', time=0.0, memory=0, status='WA'),
        dict(id=21, user_name='Ausp3x', lang='cpp', time=0.012, memory=1 << 20, status='AC')]
FIXTURE = {
    'common/random.h': '#pragma once\ninline long long mix(long long s, long long k) { return (s * k + 13) % 1000003; }\n',
    'sample/aplusb/info.toml': 'title = "A+B"\ntimelimit = 2.0\n[[tests]]\n    name = "example.in"\n    number = 2\n'
                               '[[tests]]\n    name = "random.cpp"\n    number = 3\n[params]\n    A_MAX = 1000\n    RATIO = 0.5\n'
                               '    NAME = "ab"\n',
    'sample/aplusb/gen/example_00.in': '1 2\n',
    'sample/aplusb/gen/example_01.in': '5 7\n',
    'sample/aplusb/gen/random.cpp': '#include <cstdio>\n#include <cstdlib>\n#include "random.h"\n#include "../params.h"\n'
                                    'static_assert(RATIO > 0 && sizeof(NAME) == 3);\nint main(int, char **argv) {\n'
                                    '    long long s = atoll(argv[1]);\n'
                                    '    printf("%lld %lld\\n", mix(s, 7919) % (A_MAX + 1), mix(s, 104729) % (A_MAX + 1)); }\n',
    'sample/aplusb/sol/correct.cpp': '#include <cstdio>\nint main() { long long a, b; scanf("%lld%lld", &a, &b); printf("%lld\\n", a + b); }\n',
    'sample/aplusb/checker.cpp': '#include <fstream>\n#include <string>\nint main(int, char **argv) {\n'
                                 '    std::ifstream o(argv[2]), a(argv[3]); std::string x, y;\n'
                                 '    while (true) { bool p = bool(o >> x), q = bool(a >> y); if (p != q || (p && x != y)) { return 1; } if (!p) { return 0; } } }\n',
    'sample/tiny/info.toml': 'title = "Tiny"\ntimelimit = 0.2\n[[tests]]\n    name = "example.in"\n    number = 1\n',
    'sample/tiny/gen/example_00.in': '3 4\n',
}
FIXTURE['sample/tiny/sol/correct.cpp'] = FIXTURE['sample/aplusb/sol/correct.cpp']
FIXTURE['sample/tiny/checker.cpp'] = FIXTURE['sample/aplusb/checker.cpp']
COUNTED = '#include <cstdio>\n#include <cstdlib>\n#include <unistd.h>\nint main() {\n    long long a, b; scanf("%lld%lld", &a, &b);\n' \
          '    FILE *f = fopen(getenv("COUNT"), "a+"); fseek(f, 0, SEEK_END); long n = ftell(f) / 2; fputs("r\\n", f); fclose(f);\n' \
          '    if (n % 3 == 0) { usleep(300000); }\n    printf("%lld\\n", a + b); }\n'
SOLUTIONS = {
    'ok.cpp': FIXTURE['sample/aplusb/sol/correct.cpp'],
    'wrong.cpp': '#include <cstdio>\nint main() { long long a, b; scanf("%lld%lld", &a, &b); printf("%lld\\n", a - b); }\n',
    'crash.cpp': '#include <cstdlib>\nint main() { abort(); }\n',
    'sleepy.cpp': '#include <unistd.h>\n#include <cstdio>\nint main() { usleep(500000); puts("7"); }\n',
    'spin.cpp': 'int main() { volatile unsigned x = 0; while (true) { x = x + 1; } }\n',
    'sandboxed.cpp': '#include <cerrno>\n#include <cstdio>\n#include <cstdlib>\n#include <string>\n#include <arpa/inet.h>\n'
                     '#include <sys/socket.h>\n#include <unistd.h>\nint main() {\n    long long a, b; scanf("%lld%lld", &a, &b);\n'
                     '    bool ro = !fopen((std::string(getenv("PROBE")) + "/probe").c_str(), "w") && errno == EROFS;\n'
                     '    sockaddr_in s{}; s.sin_family = AF_INET; s.sin_port = htons(80); inet_pton(AF_INET, "1.1.1.1", &s.sin_addr);\n'
                     '    bool off = connect(socket(AF_INET, SOCK_STREAM, 0), (sockaddr *)&s, sizeof s) != 0 && errno == ENETUNREACH;\n'
                     '    printf("%lld\\n", ro && off ? a + b : -1); }\n',
}


def gen_oracle(s):
    mix = lambda k: (s * k + 13) % 1000003
    return f'{mix(7919) % 1001} {mix(104729) % 1001}\n'


class Api:
    def __init__(self):
        self.urls, self.pending = [], True

    def fetch(self, url):
        self.urls.append(url)
        u = urllib.parse.urlparse(url)
        q, path = dict(urllib.parse.parse_qsl(u.query)), u.path
        if path == '/problems':
            res = {'problems': [{'name': 'aplusb', 'title': 'A+B'}, {'name': 'tiny', 'title': 'Tiny'}]}
        elif path.startswith('/problems/'):
            res = {'source_url': f'https://github.com/x/tree/master/sample/{path[10:]}', 'time_limit': 2}
        elif path == '/submissions' and 'user' in q:
            res = {'count': 2, 'submissions': [dict(s, problem_name=q['problem'], submission_time='2026-10-20T00:00:00Z') for s in MINE]}
        elif path == '/submissions':
            assert q == dict(problem='aplusb', status='AC', order='+time', dedupUser='true', limit=q['limit']), q
            res = {'count': 4, 'submissions': [dict(s, status='AC', problem_name='aplusb', **({} if s['id'] == 14 else dict(submission_time='2026-09-26T04:52:02Z')))
                                               for s in LIST[:int(q['limit'])]]}  # the oldest judge entries carry no submission_time
        else:
            sid = int(path.rsplit('/', 1)[1])
            o = next((s for s in LIST + MINE if s['id'] == sid), dict(id=sid, user_name='Late', lang='cpp', time=0.001, memory=0))
            status = o.get('status', 'AC')
            if sid == 30:
                status, self.pending = 'WJ' if self.pending else 'AC', False
            res = {'overview': dict(o, status=status, problem_name='aplusb', submission_time='2026-09-26T04:52:02Z'),
                   'source': SOURCES[sid], 'case_results': [dict(case=c, time=t, memory=1 << 20, status='AC') for c, t in CASES[sid].items()]}
        return json.dumps(res).encode()


def out(f, *args):
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        res = f(*args)
    return res, buf.getvalue()


def rows(text):
    return [[x.strip() for x in l.strip('|').split('|')] for l in text.splitlines() if l.startswith('|') and not l.startswith('|---')]


class Judge(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix='judge_tester_'))
        self.api = Api()
        self.patches = [patch.object(J, 'CACHE', self.tmp / 'cache'), patch.object(J, 'fetch', self.api.fetch),
                        patch.object(J, 'OFFLINE', False)]
        for p in self.patches:
            p.start()
        self.repo = self.tmp / 'repo'
        for rel, text in FIXTURE.items():
            (self.repo / rel).parent.mkdir(parents=True, exist_ok=True)
            (self.repo / rel).write_text(text)
        self.sol = self.tmp / 'sol'
        self.sol.mkdir()
        for name, text in SOLUTIONS.items():
            (self.sol / name).write_text(text)

    def tearDown(self):
        for p in self.patches:
            p.stop()
        shutil.rmtree(self.tmp, ignore_errors=True)

    def details(self):
        return [u for u in self.api.urls if '/submissions/' in u]

    def reproduce(self, solution, problem='aplusb', **kw):
        return out(J.reproduce, problem, self.sol / solution if isinstance(solution, str) else solution, kw.pop('label', None),
                   kw.pop('runs', 3), kw.pop('untrusted', False), str(self.repo))

    def result(self, label, problem='aplusb'):
        return json.loads((J.CACHE / 'results' / problem / f'{label}.json').read_text())

    def test_similarity(self):
        a, c = J.code_lines(A, 'cpp'), J.code_lines(A_COPY, 'cpp')
        self.assertEqual(a, c, 'comments and whitespace are ignored')
        self.assertIn('printf("%lld//x\\n",a+b);', a, 'a // inside a string literal is code')
        self.assertEqual(J.similarity(a, c), 1.0)
        b = J.code_lines(B, 'cpp')
        self.assertEqual(J.similarity(a, b), 2 / 10, '"int main() {" and "}" shared, 10 lines in the union')
        self.assertEqual(J.code_lines(PY, 'python3')['print(a+b//1)'], 1, 'non-C++ keeps //')
        self.assertEqual(J.similarity(J.code_lines('', 'cpp'), J.code_lines('', 'cpp')), 0.0)

    def test_similarity_random(self):
        for r in range(ROUNDS * 50 if MODE == 'stress' else 50):
            rng = random.Random(SEED * 1000 + r)
            n, k = rng.randint(1, 40), 0
            lines = [f'int v{i} = {rng.randrange(10**6)};' for i in range(n)]
            k = rng.randint(0, n)
            other = [f'long w{i};' for i in range(k)] + lines[k:]
            noisy = '\n'.join(('  ' * rng.randint(0, 3)) + l.replace(' = ', '=') + (' // c' if rng.random() < .3 else '') +
                              ('\n/* x\n y */' if rng.random() < .2 else '') for l in lines)
            a = J.code_lines('\n'.join(lines), 'cpp')
            with self.subTest(seed=SEED, round=r, n=n, k=k):
                self.assertEqual(J.similarity(a, J.code_lines(noisy, 'cpp')), 1.0)
                self.assertAlmostEqual(J.similarity(a, J.code_lines('\n'.join(other), 'cpp')), (n - k) / (n + k))

    def test_fastest_dedupe(self):
        kept, text = out(J.fastest, 'aplusb', 30)
        self.assertEqual([(s['id'], [c['id'] for c in copies]) for s, copies in kept], [(12, [11]), (13, []), (14, [])])
        t = rows(text)
        self.assertEqual(t[0], ['#', 'id', 'user', 'lang', 'ms', 'MiB', 'date', 'same code'])
        self.assertEqual(t[1], ['1', '12', 'Rohan_Kapri', 'cpp', '9', '3.0', '2026-09-26', '11 (anonymous)'])
        self.assertEqual(len(t), 4)
        self.assertEqual(t[3][6], '', 'an entry without submission_time prints an empty date')
        self.assertEqual(len(self.details()), 4, 'every listed source fetched once')
        out(J.fastest, 'aplusb', 2)
        self.assertEqual(len(self.details()), 4, 'final details come from the cache')
        self.assertEqual(sum('/submissions?' in u for u in self.api.urls), 2, 'lists are refetched')

    def test_submission_cache(self):
        _, text = out(J.show_submission, 13)
        path = J.CACHE / 'sources' / '13.cpp'
        self.assertEqual(path.read_text(), B)
        self.assertIn(f'source: {path} ({len(B)} bytes)', text)
        self.assertEqual(rows(text)[1:], [['example_00', '2', '1.0', 'AC'], ['random_00', '2', '1.0', 'AC'],
                                          ['random_01', '10', '1.0', 'AC'], ['judge_only_00', '3', '1.0', 'AC']])
        self.assertTrue((J.submission(14)[1]).name == '14.py')
        self.assertEqual(J.submission(30)[0]['overview']['status'], 'WJ')
        self.assertEqual(J.submission(30)[0]['overview']['status'], 'AC', 'a pending submission is refetched')
        J.submission(30)
        self.assertEqual(sum(u.endswith('/30') for u in self.api.urls), 2, 'then cached')

    def test_offline(self):
        J.submission(13)
        n = len(self.api.urls)
        with patch.object(J, 'OFFLINE', True):
            self.assertEqual(J.submission(13)[0]['source'], B)
            with self.assertRaises(SystemExit) as caught:
                J.fastest('aplusb')
        self.assertIn('--offline', str(caught.exception))
        self.assertEqual(len(self.api.urls), n)
        out(J.main, ['problems'])
        _, text = out(J.main, ['--offline', 'problems', 'tin'])
        self.assertEqual(rows(text), [['name', 'title'], ['tiny', 'Tiny']])

    def test_problems(self):
        _, text = out(J.main, ['problems', 'ap'])
        self.assertEqual(rows(text), [['name', 'title'], ['aplusb', 'A+B']])

    def test_mine(self):
        self.assertEqual(J.latest_ac('Ausp3x', 'aplusb'), 21, 'the newest AC, skipping a newer WA')
        _, text = out(J.main, ['mine', 'Ausp3x', 'aplusb'])
        t = rows(text)
        self.assertEqual([r[:2] for r in t[1:3]], [['22', 'WA'], ['21', 'AC']])
        self.assertIn('21 aplusb Ausp3x cpp AC 12 ms', text)

    def test_compare(self):
        res = J.CACHE / 'results' / 'aplusb'
        res.mkdir(parents=True)
        mk = lambda cases: json.dumps({'cases': [dict(case=c, time=t, memory=0, status=s) for c, t, s in cases]})
        (res / 'ours.json').write_text(mk([('example_00', 0.003, 'AC'), ('random_00', 0.006, 'AC'), ('random_01', 0.009, 'WA')]))
        (res / '13.json').write_text(mk([('example_00', 0.001, 'AC'), ('random_00', 0.002, 'AC'), ('random_01', 0.003, 'AC')]))
        _, text = out(J.main, ['compare', 'aplusb', '--submission', '21', '--top', '2'])
        t = rows(text)
        self.assertEqual(t[0], ['case', 'ours local', 'ours judge #21', 'Rohan_Kapri #12', 'Leader #13', 'Leader local',
                                'judge gap', 'local gap'])
        self.assertEqual(t[1], ['example_00', '3', '2', '1', '2', '1', '2.00', '3.00'])
        self.assertEqual(t[2], ['random_00', '6', '8', '4', '2', '2', '4.00', '3.00'])
        self.assertEqual(t[3], ['random_01', '9 WA', '12', '6', '10', '3', '2.00', '3.00'])
        self.assertEqual(t[4], ['judge_only_00', '—', '—', '—', '3', '—', '—', '—'])
        self.assertEqual(t[5], ['**max**', '9', '12', '6', '10', '3', '', ''])
        self.assertEqual(t[6], ['**sum**', '18', '22', '11', '17', '6', '', ''])
        self.assertIn('ours local max × 1.3 ≈ 12 ms judge estimate', text)
        _, text = out(J.main, ['compare', 'aplusb', '--user', 'Ausp3x', '--label', 'none', '--top', '1'])
        self.assertEqual(rows(text)[0], ['case', 'ours judge #21', 'Rohan_Kapri #12', 'judge gap', 'local gap'])

    def test_reproduce_ok(self):
        count = self.tmp / 'count'
        (self.sol / 'counted.cpp').write_text(COUNTED)
        with patch.dict(os.environ, COUNT=str(count)):
            bad, text = out(J.main, ['reproduce', 'aplusb', str(self.sol / 'counted.cpp'), '--repo', str(self.repo), '--label', 'ours'])
        self.assertEqual(bad, 0, text)
        tests = J.CACHE / 'tests' / 'aplusb'
        self.assertEqual((self.repo / 'sample/aplusb/params.h').read_text(), '#define A_MAX (long long)1000\n#define RATIO 0.5\n#define NAME "ab"\n')
        self.assertEqual([(tests / f'random_{s:02d}.in').read_text() for s in range(3)], [gen_oracle(s) for s in range(3)])
        self.assertEqual((tests / 'example_01.ans').read_text(), '12\n')
        r = self.result('ours')
        self.assertEqual([c['case'] for c in r['cases']], ['example_00', 'example_01', 'random_00', 'random_01', 'random_02'])
        self.assertTrue(all(c['status'] == 'AC' and 0 < c['time'] < 0.2 and 0 < c['memory'] < 8 << 20 for c in r['cases']),
                        f"{r['cases']}: a trivial solution's peak RSS excludes the tester's own")
        self.assertEqual(len(count.read_text().split()), 15, 'three runs per case; the slow first run is not the minimum')
        self.assertEqual((r['flags'], r['runs'], r['untrusted']), (J.FLAGS, 3, False))
        self.assertEqual(list((J.CACHE / 'run').iterdir()), [], 'the work directory is removed')
        self.assertIn('ours: max', text)

    def test_reproduce_failures(self):
        self.assertEqual(self.reproduce('wrong.cpp')[0], 5)
        self.assertEqual([c['status'] for c in self.result('wrong')['cases']], ['WA'] * 5)
        self.assertEqual(self.reproduce('crash.cpp', runs=1)[0], 5)
        self.assertEqual({c['status'] for c in self.result('crash')['cases']}, {'RE'})
        self.assertEqual(self.reproduce('sleepy.cpp', 'tiny', runs=1)[0], 1)
        self.assertEqual(self.result('sleepy', 'tiny')['cases'][0]['status'], 'TLE', 'wall time above the limit')
        bad, _ = self.reproduce('spin.cpp', 'tiny', runs=1)
        c = self.result('spin', 'tiny')['cases'][0]
        self.assertEqual((bad, c['status']), (1, 'TLE'))
        self.assertLess(c['time'], 5, 'the CPU limit stops a spinning solution')
        (self.sol / 'broken.cpp').write_text('int main() { return x; }\n')
        with self.assertRaises(SystemExit) as caught:
            self.reproduce('broken.cpp')
        self.assertIn('does not build', str(caught.exception))
        (self.repo / 'sample/grader').mkdir()
        (self.repo / 'sample/grader/info.toml').write_text('[[tests]]\nname = "a.in"\nnumber = 1\n[[solutions]]\nname = "c.cpp"\nfunction = true\n')
        with self.assertRaises(SystemExit) as caught:
            self.reproduce('ok.cpp', 'grader')
        self.assertIn('not supported', str(caught.exception))

    def test_regeneration_stamp(self):
        self.reproduce('ok.cpp', runs=1)
        tests = J.CACHE / 'tests' / 'aplusb'
        mtime = (tests / 'random_00.in').stat().st_mtime_ns
        self.reproduce('ok.cpp', runs=1)
        self.assertEqual((tests / 'random_00.in').stat().st_mtime_ns, mtime, 'unchanged info.toml: tests reused')
        toml = self.repo / 'sample/aplusb/info.toml'
        toml.write_text(toml.read_text().replace('number = 3', 'number = 4'))
        self.reproduce('ok.cpp', runs=1)
        self.assertEqual((tests / 'random_03.in').read_text(), gen_oracle(3))
        self.assertEqual(len(self.result('ok')['cases']), 6)

    def test_untrusted(self):
        src = J.submission(13)[1]
        with self.assertRaises(SystemExit) as caught:
            self.reproduce(src)
        self.assertIn('fetched source', str(caught.exception))
        if not shutil.which('bwrap'):
            self.skipTest('SKIP bwrap not installed')
        src.write_text(SOLUTIONS['sandboxed.cpp'])
        tests = J.CACHE / 'tests' / 'aplusb'
        self.reproduce('ok.cpp', runs=1)
        self.assertTrue(os.access(tests, os.W_OK), 'premise: the tests are writable outside bwrap')
        with patch.dict(os.environ, PROBE=str(tests)):
            bad, text = self.reproduce(src, untrusted=True, runs=1)
        self.assertEqual(bad, 0, text)
        r = self.result('13')
        self.assertTrue(r['untrusted'] and r['source'] == str(src))
        self.assertEqual({c['status'] for c in r['cases']}, {'AC'}, 'read-only tests and no network inside bwrap')
        self.assertFalse((tests / 'probe').exists())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')))
    parser.add_argument('--rounds', type=int, default=6, help='seeded random similarity rounds in stress mode')
    args = parser.parse_args()
    MODE, SEED, ROUNDS = args.mode, args.seed, args.rounds
    names = unittest.defaultTestLoader.getTestCaseNames(Judge)
    suite = unittest.TestSuite(Judge(n) for n in names if MODE != 'quick' or n in QUICK)
    print(f"judge tool checks: mode={MODE} seed={SEED} rounds={ROUNDS if MODE == 'stress' else '-'} tests={suite.countTestCases()}", flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
