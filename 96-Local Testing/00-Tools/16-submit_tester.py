#!/usr/bin/env python3
"""Network-free checks for 97-Online Testing/04-submit.py, the maintainer's Library Checker submission tool.

A local HTTP server stands in for the judge's frontend configuration, Firebase sign-in and token refresh, and the REST API
(current user, submit, submission details): a submission reads -, Compiling and 2/9 on its first fetches, then AC, or WA when its source
contains WRONG; held submissions #7 and #8 serve fixed sources. The library is a temporary git repository with src/ and
expanded/ fixtures; the expander is replaced by a copy of src into expanded/ (01-expander.py itself: 02-online_tester.py).
Oracles: the server's request log, the saved configuration and its mode, hand-written Status lines, and a direct
recomputation of selections.
quick: login and token refresh, states, selection, the AC flow and its record, green and red verdicts on a terminal only.
full: also WA, refusal at the confirmation, missing or mismatched logins, record rewrites, a failing expander, +dirty,
ID tokens renewed by age, network errors retried while waiting and reported without a traceback elsewhere, and main()
refused in a shell Claude Code started unless the user's override is set (test_claude_shell_guard).
stress: full plus --rounds x 200 seeded random selections against the oracle.
"""
import argparse
import contextlib
import io
import fnmatch
import hashlib
import http.server
import importlib.util
import json
import os
import random
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import unittest
import urllib.parse
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap
import time

_00_memory_cap.ensure()

spec = importlib.util.spec_from_file_location('submit', Path(__file__).resolve().parents[2] / '97-Online Testing/04-submit.py')
S = importlib.util.module_from_spec(spec)
spec.loader.exec_module(S)
MODE, SEED, ROUNDS = 'full', 42, 3
QUICK = {'test_login', 'test_refresh', 'test_states', 'test_pick', 'test_submit_ac', 'test_colors'}

HEAD = '// PROBLEM: https://judge.yosupo.jp/problem/{}\n'
BODY = '#include <cstdio>\nint main() { puts("{}"); }\n'
HELD = {7: HEAD.format('aplusb') + '// Status: old status text\n' + BODY, 8: HEAD.format('unionfind') + '// Status: x\n' + BODY + '// older\n'}


class Judge(http.server.BaseHTTPRequestHandler):
    """The fake services; state lives on the server object: log, user, subs {id: [source, fetches, lang]}, next id."""
    def log_message(self, *a):
        pass

    def reply(self, data, code=200):
        body = (data if isinstance(data, str) else json.dumps(data)).encode()
        self.send_response(code)
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def token(self):
        return self.headers.get('Authorization', '').removeprefix('Bearer ')

    def do_GET(self):
        srv, path = self.server, urllib.parse.urlsplit(self.path).path
        srv.log.append(('GET', path))
        if path == '/env':
            return self.reply('VITE_API_URL=x\nVITE_FIREBASE_API_KEY=key1\n')
        if path == '/auth/current_user':
            return self.reply({'user': {'name': srv.user, 'library_url': '', 'is_developer': False}} if self.token() in ('id1', 'id2') and srv.user else {})
        m = re.fullmatch(r'/submissions/(\d+)', path)
        if m and int(m[1]) in srv.subs:
            sub = srv.subs[int(m[1])]
            sub[1] += 1
            steps = ['-', 'Compiling', '2/9']
            status = steps[sub[1] - 1] if sub[1] <= len(steps) and int(m[1]) not in HELD else 'WA' if 'WRONG' in sub[0] else 'AC'
            return self.reply({'overview': {'id': int(m[1]), 'status': status, 'lang': sub[2], 'time': 0.0431,
                                            'submission_time': '2026-11-15T01:02:03Z'}, 'source': sub[0]})
        self.reply({'message': 'not found'}, 404)

    def do_POST(self):
        srv = self.server
        url = urllib.parse.urlsplit(self.path)
        raw = self.rfile.read(int(self.headers.get('Content-Length', 0))).decode()
        srv.log.append(('POST', url.path, raw, self.token()))
        query = urllib.parse.parse_qs(url.query)
        if url.path == '/signin':
            body = json.loads(raw)
            if query.get('key') == ['key1'] and body == {'email': 'me@x', 'password': 'pw', 'returnSecureToken': True}:
                return self.reply({'idToken': 'id1', 'refreshToken': 'r1'})
            return self.reply({'error': 'INVALID_PASSWORD'}, 400)
        if url.path == '/refresh':
            form = urllib.parse.parse_qs(raw)
            if query.get('key') == ['key1'] and form == {'grant_type': ['refresh_token'], 'refresh_token': [srv.refresh]}:
                srv.refresh = 'r2'
                return self.reply({'id_token': 'id2', 'refresh_token': 'r2'})
            return self.reply({'error': 'INVALID_REFRESH_TOKEN'}, 400)
        if url.path == '/submit':
            if self.token() != 'id2':
                return self.reply({'message': 'unauthenticated'}, 401)
            body = json.loads(raw)
            srv.next += 1
            srv.subs[srv.next] = [body['source'], 0, body['lang']]
            return self.reply({'id': srv.next})
        self.reply({'message': 'not found'}, 404)


class Submit(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix='submit tester '))
        self.server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Judge)
        self.server.log, self.server.user, self.server.refresh, self.server.next = [], 'Tester', 'r1', 100
        self.server.subs = {k: [v, 0, 'cpp'] for k, v in HELD.items()}
        threading.Thread(target=self.server.serve_forever, daemon=True).start()
        url = f'http://127.0.0.1:{self.server.server_address[1]}'
        lib = self.tmp / 'library'
        self.root = lib / '97-Online Testing'
        files = {
            'src/01-Yosupo/01-A/01-aplusb.cpp': HEAD.format('aplusb') + '// Status: locally run (2026-11-15); online acceptance not verified.\n' + BODY,
            'src/01-Yosupo/01-A/02-unionfind.cpp': HEAD.format('unionfind') + '// Status: accepted online (#8, cpp, 2026-11-01 UTC, 5 ms; the expanded bundle of commit abc, SHA-256 0123456789abcdef); locally run.\n' + BODY,
            'src/01-Yosupo/02-B/01-lca.cpp': HEAD.format('lca') + BODY,
            'src/01-Yosupo/02-B/02-wrong.cpp': HEAD.format('unionfind') + '// WRONG\n' + BODY,
            'src/02-CSES/01-x.cpp': '// PROBLEM: https://cses.fi/problemset/task/1068\n' + BODY,
            'src/01-Yosupo/02-B/03-stray.expanded.cpp': HEAD.format('aplusb') + BODY,
        }
        for rel, text in files.items():
            (self.root / rel).parent.mkdir(parents=True, exist_ok=True)
            (self.root / rel).write_text(text)
        (lib / '01-Core').mkdir()
        (lib / '01-Core/01-template.hpp').write_text('#pragma once\n')
        for cmd in (['init', '-q'], ['add', '-A'], ['-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-q', '-m', 'fixture']):
            subprocess.run(['git', '-C', str(lib), *cmd], check=True, capture_output=True)
        self.saved = {k: getattr(S, k) for k in ('ROOT', 'LIBRARY', 'SRC', 'EXPANDED', 'CONFIG', 'FRONTEND_ENV', 'SIGN_IN', 'REFRESH', 'POLL', 'WAIT', 'TOKEN_AGE', 'expand')}
        self.judge = {k: getattr(S.J, k) for k in ('API', 'CACHE', 'submission')}
        S.ROOT, S.LIBRARY, S.SRC, S.EXPANDED = self.root, lib, self.root / 'src', self.root / 'expanded'
        S.CONFIG, S.FRONTEND_ENV, S.SIGN_IN, S.REFRESH, S.POLL = self.tmp / 'cfg/yosupo.json', url + '/env', url + '/signin', url + '/refresh', 0.01
        S.expand = self.fake_expand
        S.J.API, S.J.CACHE = url, self.tmp / 'cache'
        self.fake_expand()

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        for k, v in self.saved.items():
            setattr(S, k, v)
        for k, v in self.judge.items():
            setattr(S.J, k, v)
        shutil.rmtree(self.tmp, ignore_errors=True)

    def fake_expand(self):
        """expanded/<rel>.expanded.cpp = '// bundle\\n' + the source, for every unexpanded source."""
        for p in (self.root / 'src').rglob('*.cpp'):
            if not p.name.endswith('.expanded.cpp'):
                out = self.root / 'expanded' / p.relative_to(self.root / 'src').as_posix().replace('.cpp', '.expanded.cpp')
                out.parent.mkdir(parents=True, exist_ok=True)
                out.write_text('// bundle\n' + p.read_text())

    def login(self):
        answers = iter(['me@x '])
        S.login(ask=lambda _: next(answers), secret=lambda _: 'pw')

    def posts(self, path):
        return [e for e in self.server.log if e[0] == 'POST' and e[1] == path]

    def test_login(self):
        self.login()
        cfg = json.loads(S.CONFIG.read_text())
        self.assertEqual(cfg, {'api_key': 'key1', 'refresh_token': 'r1', 'user': 'Tester'}, 'the password is never stored')
        self.assertEqual(S.CONFIG.stat().st_mode & 0o777, 0o600)
        self.assertEqual(S.CONFIG.parent.stat().st_mode & 0o777, 0o700)
        with self.assertRaises(SystemExit) as caught:
            S.login(ask=lambda _: 'me@x', secret=lambda _: 'bad')
        self.assertIn('INVALID_PASSWORD', str(caught.exception))
        self.assertEqual(json.loads(S.CONFIG.read_text())['refresh_token'], 'r1', 'a failed login keeps the old one')
        self.server.user = None
        with self.assertRaises(SystemExit) as caught:
            self.login()
        self.assertIn('register', str(caught.exception), 'a login with no judge user is refused')

    def test_refresh(self):
        with self.assertRaises(SystemExit) as caught:
            S.id_token()
        self.assertIn('login', str(caught.exception), 'no saved login: no anonymous submission')
        self.login()
        self.assertEqual(S.id_token(), ('id2', 'Tester'))
        self.assertEqual(json.loads(S.CONFIG.read_text())['refresh_token'], 'r2', 'a rotated refresh token is saved')
        self.assertEqual(S.id_token(), ('id2', 'Tester'), 'the rotated token works')
        self.server.user = 'Someone'
        with self.assertRaises(SystemExit) as caught:
            S.id_token()
        self.assertIn('not Tester', str(caught.exception))

    def test_states(self):
        rows = S.sources()
        self.assertEqual([(r[0], r[1]) for r in rows], [('01-Yosupo/01-A/01-aplusb.cpp', 'aplusb'), ('01-Yosupo/01-A/02-unionfind.cpp', 'unionfind'),
                                                        ('01-Yosupo/02-B/01-lca.cpp', 'lca'), ('01-Yosupo/02-B/02-wrong.cpp', 'unionfind')],
                         'Yosupo sources only, expanded files under src skipped')
        self.assertEqual([S.state(p, b) for _, _, p, b in rows], [('new', None), ('stale', '8'), ('new', None), ('new', None)])
        held = self.root / 'src/01-Yosupo/02-B/01-lca.cpp'
        held.write_text(HEAD.format('lca') + '// Status: accepted online (#7, cpp, 2026-11-01 UTC, 1 ms; the expanded bundle of commit abc, SHA-256 0)\n' + BODY)
        self.server.subs[7][0] = '// bundle\r\n' + HEAD.format('lca') + '// Status: something else  \n' + BODY + '\n\n'
        self.fake_expand()
        self.assertEqual(S.state(held, rows[2][3]), ('current', '7'), 'Status lines, CR and trailing whitespace do not count')
        self.assertEqual(S.strip_status('a \r\n// Status: x\nb\n\n'), 'a\nb')

    def test_pick(self):
        rows = S.sources()
        states = [('new', None), ('stale', '8'), ('current', '7'), ('new', None)]
        cases = {('2',): [1], ('2-3', '1'): [1, 2, 0], ('3', '2-4'): [2, 1, 3], ('unionfind',): [1, 3], ('new',): [0, 3], ('stale',): [1],
                 ('all',): [0, 1, 2, 3], ('*/02-B/*',): [2, 3], ('01-Yosupo/02-B/01-lca.cpp',): [2], ('src/01-Yosupo/02-B/01-lca.cpp',): [2],
                 ('lca', 'new'): [2, 0, 3], ('wrong',): [3], ('02-wrong',): [3], ('02-wrong.cpp', 'aplusb'): [3, 0], (): []}
        for words, want in cases.items():
            self.assertEqual(S.pick(list(words), rows, states), want, words)
        for words in (['0'], ['5'], ['3-9'], ['nothing'], ['*.py'], ['wron'], ['03-wrong']):
            with self.assertRaises(SystemExit):
                S.pick(words, rows, states)

    def test_submit_ac(self):
        self.login()
        src = self.root / 'src/01-Yosupo/01-A/01-aplusb.cpp'
        sent = (self.root / 'expanded/01-Yosupo/01-A/01-aplusb.expanded.cpp').read_bytes()
        self.assertEqual(S.submit(['aplusb'], yes=True), 0)
        (sub,) = self.posts('/submit')
        self.assertEqual(json.loads(sub[2]), {'problem': 'aplusb', 'source': sent.decode(), 'lang': 'cpp'})
        self.assertEqual(sub[3], 'id2', 'the refreshed ID token is the bearer')
        head = subprocess.run(['git', '-C', str(S.LIBRARY), 'rev-parse', '--short', 'HEAD'], capture_output=True, text=True).stdout.strip()
        want = (f'// Status: accepted online (#101, cpp, 2026-11-15 UTC, 43 ms; the expanded bundle of commit {head}, '
                f'SHA-256 {hashlib.sha256(sent).hexdigest()[:16]}); locally run (2026-11-15).')
        self.assertEqual(src.read_text().split('\n')[1], want, 'the record replaces the not-verified clause')
        self.assertEqual(self.server.subs[101][1], 4, 'waited through -, Compiling and 2/9 to the verdict')
        self.assertEqual(len(self.posts('/refresh')), 1, 'one ID token while it is young')
        self.fake_expand()
        rows = S.sources()
        self.assertEqual(S.state(src, rows[0][3]), ('current', '101'), 'writing the record does not make the source stale')

    def test_submit_wa_and_confirm(self):
        self.login()
        src = self.root / 'src/01-Yosupo/02-B/02-wrong.cpp'
        before = src.read_text()
        self.assertEqual(S.submit(['4'], ask=lambda _: 'n'), 0)
        self.assertEqual(self.posts('/submit'), [], 'no submission without a yes')
        self.assertEqual(S.submit([], ask=lambda _: ''), 0, 'an empty selection quits')
        self.assertEqual(self.posts('/submit'), [])
        answers = iter(['4', 'y'])
        self.assertEqual(S.submit([], ask=lambda _: next(answers)), 1, 'a WA counts as a failure')
        self.assertEqual(src.read_text(), before, 'no record for a WA')

    def test_colors(self):
        self.login()
        tty = type('Tty', (io.StringIO,), {'isatty': lambda self: True})
        for out, ac, wa, total in ((tty(), '\033[32mAC\033[0m', '\033[31mWA\033[0m', '\033[31m1 accepted, 1 not\033[0m'),
                                   (io.StringIO(), ' AC ', ' WA ', '1 accepted, 1 not; AC')):
            with contextlib.redirect_stdout(out):
                self.assertEqual(S.submit(['aplusb', 'wrong'], yes=True), 1)
            text = out.getvalue()
            for want in (ac, wa, total):
                self.assertIn(want, text)
            self.assertEqual('\033' in text, isinstance(out, tty), 'colors on a terminal only')
        out = tty()
        with contextlib.redirect_stdout(out):
            S.submit(['aplusb'], yes=True)
        self.assertIn('\033[32m1 accepted, 0 not\033[0m', out.getvalue())
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            S.submit(['wrong'], yes=True)
        self.assertIn('0 accepted, 1 not.\n', out.getvalue(), 'no record note without an AC')

    def test_token_age(self):
        self.login()
        S.TOKEN_AGE = -1.0
        self.assertEqual(S.submit(['aplusb', 'lca'], yes=True), 0)
        self.assertEqual(len(self.posts('/refresh')), 2, 'an old ID token is replaced before each submission')
        self.assertEqual([e[3] for e in self.posts('/submit')], ['id2', 'id2'])

    def test_wait_retries(self):
        self.login()
        real, failures = S.J.submission, [ConnectionResetError('reset by peer'), TimeoutError('timed out')]
        def flaky(sid):
            if sid == 101 and failures:
                raise failures.pop(0)
            return real(sid)
        S.J.submission = flaky
        self.assertEqual(S.submit(['aplusb'], yes=True), 0, 'network errors while waiting are retried')
        self.assertEqual(failures, [], 'both errors were raised')
        self.assertIn('accepted online (#101', (self.root / 'src/01-Yosupo/01-A/01-aplusb.cpp').read_text())
        S.J.submission = lambda sid: (_ for _ in ()).throw(ConnectionRefusedError('refused'))
        S.WAIT = 0.0
        start = time.monotonic()
        with self.assertRaises(SystemExit) as caught:
            S.wait(5)
        self.assertIn('still unreachable (refused)', str(caught.exception))
        with self.assertRaises(SystemExit) as caught:
            S.state(self.root / 'src/01-Yosupo/01-A/02-unionfind.cpp', self.root / 'expanded/01-Yosupo/01-A/02-unionfind.expanded.cpp')
        self.assertIn('#8, recorded in 02-unionfind.cpp: refused', str(caught.exception), 'an unreachable judge stops the listing without a traceback')
        self.assertLess(time.monotonic() - start, 5, 'the wait limit holds while unreachable')
        S.REFRESH = 'http://127.0.0.1:9/refresh'
        with self.assertRaises(SystemExit) as caught:
            S.id_token()
        self.assertIn('127.0.0.1:9/refresh', str(caught.exception), 'an unreachable service exits with its address, no traceback')

    def test_dirty(self):
        self.login()
        (S.LIBRARY / '01-Core/01-template.hpp').write_text('#pragma once\n// edited\n')
        S.submit(['lca'], yes=True)
        line = (self.root / 'src/01-Yosupo/02-B/01-lca.cpp').read_text().split('\n')[1]
        self.assertRegex(line, r'^// Status: accepted online \(#101, cpp, 2026-11-15 UTC, 43 ms; the expanded bundle of commit \w+\+dirty, SHA-256 [0-9a-f]{16}\)\.$',
                         'a library edit marks the commit dirty; a source without a Status line gets one after PROBLEM')

    def test_record_rewrites(self):
        o = {'submission_time': '2026-11-15T01:02:03Z', 'time': 1.5}
        p = self.tmp / 'r.cpp'
        for before, rest in (('// Status: migrated source; online acceptance not verified in this migration.', '; migrated source.'),
                             ('// Status: accepted online (#5, cpp, 2026-01-01 UTC, 9 ms; the expanded bundle of commit a, SHA-256 b); locally run.', '; locally run.'),
                             ('// Status: locally run; the old revision was accepted online (#4, cpp GCC 15.2, 2026-10-31).', '; locally run; the old revision was accepted online (#4, cpp GCC 15.2, 2026-10-31).'),
                             ('// Status: online acceptance not verified.', '.')):
            p.write_text(HEAD.format('aplusb') + before + '\n' + BODY)
            S.record(p, 9, 'cpp', o, b'x', 'abc')
            acc = f'// Status: accepted online (#9, cpp, 2026-11-15 UTC, 1500 ms; the expanded bundle of commit abc, SHA-256 {hashlib.sha256(b"x").hexdigest()[:16]})'
            self.assertEqual(p.read_text(), HEAD.format('aplusb') + acc + rest + '\n' + BODY, before)

    def test_expander_failure(self):
        S.expand = self.saved['expand']
        (self.root / '01-expander.py').write_text('import sys\nprint("boom")\nsys.exit(3)\n')
        self.login()
        with self.assertRaises(SystemExit) as caught:
            S.submit(['all'], yes=True)
        self.assertIn('boom', str(caught.exception))
        self.assertEqual(self.posts('/submit'), [], 'nothing submitted after a failed expansion')

    def test_claude_shell_guard(self):
        """main() refuses in a shell Claude Code started unless CP_USER=1 (SUP32), before any argument is read."""
        S.LIBRARY = self.saved['LIBRARY']  # the real .claude/hooks/shell.py
        refused = ("04-submit.py refused: this shell was started by Claude Code, and submitting is the user's "
                   "(01-principles.md, Scope Discipline). Typed yourself with `!`? Prefix CP_USER=1.")
        saved = {k: os.environ.get(k) for k in ('CLAUDE_CODE_CHILD_SESSION', 'CP_USER')}
        try:
            for child, user, ok in (('1', None, False), ('1', '1', True), ('1', '0', False), (None, None, True), ('0', None, True)):
                for k, v in (('CLAUDE_CODE_CHILD_SESSION', child), ('CP_USER', user)):
                    os.environ.pop(k, None) if v is None else os.environ.__setitem__(k, v)
                with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as caught:
                    S.main(['--no-such-option'])
                self.assertEqual(caught.exception.code, 2 if ok else refused, (child, user))
        finally:
            for k, v in saved.items():
                os.environ.pop(k, None) if v is None else os.environ.__setitem__(k, v)
        self.assertEqual(self.posts('/submit'), [])

    def test_random_pick(self):
        if MODE != 'stress':
            self.skipTest('stress only')
        rng = random.Random(SEED)
        rows = S.sources()
        for _ in range(ROUNDS * 200):
            states = [(rng.choice(['new', 'current', 'stale']), None) for _ in rows]
            pool = [str(i) for i in range(1, len(rows) + 1)] + ['all', 'new', 'stale', 'unionfind', 'lca', 'wrong', '01-lca', '02-wrong.cpp', '*/01-A/*', '1-3', '2-4']
            words = rng.sample(pool, rng.randint(1, 4))
            want = []
            for w in words:
                if re.fullmatch(r'\d+-\d+', w):
                    a, b = map(int, w.split('-'))
                    hits = list(range(a - 1, b))
                elif w.isdigit():
                    hits = [int(w) - 1]
                elif w in ('all', 'new', 'stale'):
                    hits = [i for i, s in enumerate(states) if w == 'all' or s[0] == w]
                else:
                    hits = [i for i, r in enumerate(rows) if r[1] == w or fnmatch.fnmatch(r[0], w) or w in (r[2].name, r[2].stem, r[2].stem[3:])]
                want += [i for i in hits if i not in want]
            self.assertEqual(S.pick(words, rows, states), want, (words, states))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')))
    parser.add_argument('--rounds', type=int, default=3, help='x 200 seeded random selections in stress mode')
    args = parser.parse_args()
    MODE, SEED, ROUNDS = args.mode, args.seed, args.rounds
    names = unittest.defaultTestLoader.getTestCaseNames(Submit)
    suite = unittest.TestSuite(Submit(n) for n in names if MODE != 'quick' or n in QUICK)
    print(f"submit tool checks: mode={MODE} seed={SEED} rounds={ROUNDS if MODE == 'stress' else '-'} tests={suite.countTestCases()}", flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
