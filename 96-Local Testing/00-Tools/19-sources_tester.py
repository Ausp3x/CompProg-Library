#!/usr/bin/env python3
"""Checks for 95-Resources/00-sources.py (filing, the Storage check, fetch and the private backup; 00-index.md, Storage).

Each test builds a fixture repository whose 95-Resources holds a copy of the script, its private remote replaced by a
local bare repository, a manifest of one filed paper and git-ignored 00-Inbox, 13-Papers and 14-Web Pages; the real
95-Resources and its private history are never touched. quick: file and check. full: quick plus a linked worktree,
fetch and the private backup. stress: as full. Modes also come from CP_TEST_MODE.
Oracle: file contents, manifest entries and check lines written by hand from 00-index.md (Storage, Filing) and the
script's docstring; git's own record of the private history.

Feature map:
file moves an inbox file into 13-Papers or 14-Web Pages, adds the manifest entry with every field, keeps the manifest's
format and non-ASCII text, deletes only its own request line, records the user as provider -> test_file_from_inbox;
refusals (folder, name pattern, the same bytes or path already in the manifest, an unlisted file at the destination, a
missing source, missing options) leave everything in place -> test_file_refusals;
a session's own download is provided by the session, --provided-by overrides, and with a private history the file is
committed there at once with the push command printed -> test_file_backup;
check: inbox files placed for the given ids, without a request line or copying a filed file reported and another
session's request left alone; citations of inbox files in tracked and new Markdown, not in fences, OLD, 95-Resources or
git-ignored files and not `00-Inbox/00-requests.md` or the bare folder; filed files without an entry; exit codes ->
test_check; with a private history: manifest files missing here and filed files not backed up -> test_check_backup;
a linked worktree files into the main checkout and its own manifest, and every checkout's manifest counts ->
test_worktree; fetch moves a file the user placed in 00-Inbox with a missing entry's SHA-256 into place and drops its
request line, downloads a missing entry by its URL and checks its SHA-256, names the entries without a URL in one line
without failing, and fails on a wrong download or a changed local file, which check reports as the entry's file ->
test_fetch.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / '95-Resources/00-sources.py'
REMOTE_LINE = "REMOTE = 'https://github.com/Ausp3x/CompProg-Library-Sources.git'"
LOCAL = 'Personal study copy; terms do not permit public redistribution, so the file stays local (git-ignored).'
OLD = b'old paper\n'
ENTRY = {'file': '13-Papers/01-old_paper-a_2000.pdf', 'title': 'A, *Old*', 'source_page': 'https://example.invalid/old',
         'downloaded': '2026-11-22', 'bytes': len(OLD), 'sha256': hashlib.sha256(OLD).hexdigest(),
         'cited_in': 'Docs/a.md:1', 'provided_by': 'user', 'reuse_note': LOCAL, 'committed': False}
ENV = {k: v for k, v in os.environ.items() if not k.startswith('GIT_')} | {
    'GIT_CONFIG_GLOBAL': os.devnull, 'GIT_CONFIG_SYSTEM': os.devnull, 'GIT_CONFIG_NOSYSTEM': '1',
    'GIT_AUTHOR_NAME': 'T', 'GIT_AUTHOR_EMAIL': 't@example.invalid', 'GIT_COMMITTER_NAME': 'T', 'GIT_COMMITTER_EMAIL': 't@example.invalid'}
MODE = 'full'


def git(cwd, *args):
    r = subprocess.run(['git', *args], cwd=cwd, capture_output=True, text=True, env=ENV)
    if r.returncode:
        raise RuntimeError(f'git {" ".join(args)} failed in {cwd}: {r.stderr}')
    return r.stdout


def sha(data):
    return hashlib.sha256(data).hexdigest()


class Sources(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix='cp-sourcestest-'))
        self.addCleanup(shutil.rmtree, self.tmp, True)
        self.main, self.remote = self.tmp / 'main', self.tmp / 'private.git'
        self.res = self.main / '95-Resources'
        (self.res / '13-Papers').mkdir(parents=True)
        text = SCRIPT.read_text()
        self.assertEqual(text.count(REMOTE_LINE), 1, 'the private remote line moved')
        (self.res / '00-sources.py').write_text(text.replace(REMOTE_LINE, f'REMOTE = {str(self.remote)!r}'))
        self.write_manifest(self.res, [ENTRY])
        (self.res / ENTRY['file']).write_bytes(OLD)
        (self.main / '.gitignore').write_text('95-Resources/00-Inbox\n95-Resources/13-Papers\n95-Resources/14-Web Pages\n'
                                              '__pycache__/\nignored/\n.claude/worktrees/\n')
        (self.main / 'Docs').mkdir()
        (self.main / 'Docs/a.md').write_text('# A\n')
        git(self.tmp, 'init', '-q', '--bare', '-b', 'master', str(self.remote))
        git(self.main, 'init', '-q', '-b', 'master')
        git(self.main, 'add', '-A')
        git(self.main, 'commit', '-q', '-m', 'init')
        self.inbox = self.res / '00-Inbox'
        (self.inbox / 'Papers').mkdir(parents=True)

    def full(self):
        if MODE == 'quick':
            self.skipTest('full mode only')

    @staticmethod
    def write_manifest(res, entries):
        (res / '99-sources.json').write_text(json.dumps({'description': 'Reference files – test', 'resources': entries},
                                                        indent=2, ensure_ascii=False) + '\n')

    def manifest(self, res=None):
        return json.loads(((res or self.res) / '99-sources.json').read_text())['resources']

    def run_script(self, *args, res=None):
        return subprocess.run([sys.executable, str((res or self.res) / '00-sources.py'), *args], cwd=self.main,
                              capture_output=True, text=True, env=ENV, timeout=120)

    def place(self, name, data):
        path = self.inbox / 'Papers' / name
        path.write_bytes(data)
        return path

    def file(self, src, dest, *extra, res=None):
        return self.run_script('file', str(src), dest, '--title', 'R–W, *Coolest*', '--url', 'https://example.invalid/p',
                               '--cited-in', 'Docs/a.md:3', *extra, res=res)

    def private(self):
        r = self.run_script('init')
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn('has no master branch yet', r.stdout)
        return ['git', f'--git-dir={self.main}/.git/sources']

    def test_file_from_inbox(self):
        src = self.place('x.pdf', b'paper\n')
        head = '# Requests\n\nOne line per open request.\n'
        mine = '- 2026-11-27 | P900 | X | https://example.invalid/x | x.pdf'
        other = '- 2026-11-27 | P901 | Y | https://example.invalid/y | y.pdf'
        (self.inbox / '00-requests.md').write_text(f'{head}{mine}\n{other}\n')
        r = self.file(src, '13-Papers/02-coolest-ruskey_2009.pdf', '--pages', '3')
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn('filed 13-Papers/02-coolest-ruskey_2009.pdf', r.stdout)
        self.assertNotIn('backed up', r.stdout, 'no private history')
        self.assertFalse(src.exists(), 'moved, not copied')
        self.assertEqual((self.res / '13-Papers/02-coolest-ruskey_2009.pdf').read_bytes(), b'paper\n')
        want = {'file': '13-Papers/02-coolest-ruskey_2009.pdf', 'title': 'R–W, *Coolest*', 'source_page': 'https://example.invalid/p',
                'downloaded': str(datetime.date.today()), 'bytes': 6, 'pages': 3, 'sha256': sha(b'paper\n'), 'cited_in': 'Docs/a.md:3',
                'provided_by': 'user', 'reuse_note': LOCAL, 'committed': False}
        self.assertEqual(self.manifest(), [ENTRY, want])
        text = (self.res / '99-sources.json').read_text()
        self.assertEqual(text, json.dumps(json.loads(text), indent=2, ensure_ascii=False) + '\n', 'format and non-ASCII kept')
        self.assertEqual((self.inbox / '00-requests.md').read_text(), f'{head}{other}\n', 'only its own request line goes')
        r = self.run_script('file', str(self.place('y.pdf', b'y\n')), '14-Web Pages/03-site_1-page.pdf', '--title', 'Y', '--url', 'u',
                            '--cited-in', 'Docs/a.md:4', '--download-url', 'https://example.invalid/y.pdf')
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(self.manifest()[-1]['download_url'], 'https://example.invalid/y.pdf')
        self.assertTrue((self.res / '14-Web Pages/03-site_1-page.pdf').is_file())
        self.assertEqual((self.inbox / '00-requests.md').read_text(), head)

    def test_file_refusals(self):
        src = self.place('x.pdf', b'paper\n')
        before = (self.res / '99-sources.json').read_text()
        (self.res / '13-Papers/03-taken-a_1.pdf').write_bytes(b'unlisted\n')
        for dest, why in (('15-Other/02-x-a_1.pdf', 'name the destination'), ('13-Papers/02-Bad-a_1.pdf', 'name the destination'),
                          ('13-Papers/x-a_1.pdf', 'name the destination'), ('13-Papers/02-x-a_1', 'name the destination'),
                          (ENTRY['file'], f'already holds {ENTRY["file"]}'),
                          ('13-Papers/03-taken-a_1.pdf', 'exists without a manifest entry')):
            r = self.file(src, dest)
            self.assertNotEqual(r.returncode, 0, dest)
            self.assertIn(why, r.stderr, dest)
        r = self.file(self.place('dup.pdf', OLD), '13-Papers/04-dup-a_1.pdf')
        self.assertIn(f'already holds {ENTRY["file"]}, with these bytes: cite it and delete this copy', r.stderr)
        r = self.file(self.inbox / 'missing.pdf', '13-Papers/05-m-a_1.pdf')
        self.assertIn('no file', r.stderr)
        r = self.run_script('file', str(src), '13-Papers/06-y-a_1.pdf', '--title', 'T', '--url', 'u')
        self.assertEqual(r.returncode, 2, 'argparse error without --cited-in')
        self.assertTrue(src.exists() and (self.inbox / 'Papers/dup.pdf').exists(), 'nothing moved')
        self.assertEqual((self.res / '99-sources.json').read_text(), before, 'manifest unchanged')

    def test_file_backup(self):
        self.full()
        priv = self.private()
        scratch = self.tmp / 'download.pdf'
        scratch.write_bytes(b'download\n')
        r = self.file(scratch, '13-Papers/02-d-a_1.pdf')
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(self.manifest()[-1]['provided_by'], 'session', "a session's own download")
        self.assertIn('backed up in the private repository; push (the user runs it):', r.stdout)
        self.assertIn('push -u origin master', r.stdout)
        log = subprocess.run([*priv, 'log', '--format=%s', '--name-only', '-1'], capture_output=True, text=True, env=ENV).stdout
        self.assertEqual(log.split(), ['File', '13-Papers/02-d-a_1.pdf', '13-Papers/02-d-a_1.pdf'])
        r = self.file(self.place('c.pdf', b'curl\n'), '13-Papers/03-c-a_1.pdf', '--provided-by', 'session (curl)')
        self.assertEqual((r.returncode, self.manifest()[-1]['provided_by']), (0, 'session (curl)'), r.stderr)

    def test_check(self):
        r = self.run_script('check')
        self.assertEqual((r.returncode, r.stdout), (0, 'sources: nothing to file\n'), r.stderr)
        (self.inbox / '00-requests.md').write_text('# Requests\n\n- 2026-11-27 | P900 | A | u | a.pdf\n- 2026-11-27 | P901 | B | u | b.pdf\n')
        for name, data in (('a.pdf', b'a\n'), ('b.pdf', b'b\n'), ('c.pdf', b'c\n'), ('d.pdf', OLD)):
            self.place(name, data)
        cite = '`95-Resources/00-Inbox/Papers/a.pdf`'
        (self.main / 'Docs/a.md').write_text(f'# A\n\nSee {cite}.\nThe log `00-Inbox/00-requests.md` and the folder `00-Inbox/`.\n\n'
                                             f'```bash\nls {cite}\n```\n')
        (self.main / 'Docs/new.md').write_text(f'New {cite}\n')
        for kept in ('OLD', 'ignored'):
            (self.main / kept).mkdir()
            (self.main / kept / 'x.md').write_text(f'{cite}\n')
        (self.res / 'notes.md').write_text(f'{cite}\n')
        (self.res / '13-Papers/09-z-a_1.pdf').write_bytes(b'z\n')
        r = self.run_script('check', 'P900')
        self.assertEqual(r.returncode, 1, r.stderr)
        self.assertEqual(r.stdout.splitlines(), [
            '00-Inbox/Papers/a.pdf: placed for P900; file it (00-sources.py file)',
            '00-Inbox/Papers/c.pdf: no request line; file it if this work used it (00-sources.py file), else ask the user',
            f'00-Inbox/Papers/d.pdf: a copy of the filed {ENTRY["file"]}; delete it',
            'Docs/a.md:3: cites a file in 00-Inbox; file it and cite the filed path',
            'Docs/new.md:1: cites a file in 00-Inbox; file it and cite the filed path',
            '13-Papers/09-z-a_1.pdf: no manifest entry'])
        r = self.run_script('check', 'P901', 'MI16')
        self.assertIn('00-Inbox/Papers/b.pdf: placed for P901; file it (00-sources.py file)', r.stdout)
        self.assertNotIn('a.pdf: placed', r.stdout, "another session's request stays")

    def test_check_backup(self):
        self.full()
        two = dict(ENTRY, file='13-Papers/02-two-a_1.pdf', sha256=sha(b'two\n'))
        self.write_manifest(self.res, [ENTRY, two])
        (self.res / two['file']).write_bytes(b'two\n')
        priv = self.private()
        self.assertEqual(self.run_script('check').stdout, 'filed files not backed up (2): 00-sources.py commit\n', 'one per file')
        subprocess.run([*priv, 'add', '-A'], check=True, env=ENV)
        subprocess.run([*priv, 'commit', '-q', '-m', 'all'], check=True, env=ENV)
        self.assertEqual(self.run_script('check').stdout, 'sources: nothing to file\n')
        (self.res / ENTRY['file']).unlink()
        self.assertEqual(self.run_script('check').stdout.splitlines(), [f'{ENTRY["file"]}: in the manifest, missing here',
                                                                        'filed files not backed up (1): 00-sources.py commit'])

    def test_worktree(self):
        self.full()
        wt = self.main / '.claude/worktrees/W'
        git(self.main, 'worktree', 'add', '-q', '-b', 'w', str(wt))
        src = self.place('x.pdf', b'x\n')
        (wt / 'Docs/w.md').write_text('`95-Resources/00-Inbox/Papers/x.pdf`\n')
        r = self.run_script('check', res=wt / '95-Resources')
        self.assertIn('Docs/w.md:1: cites a file in 00-Inbox; file it and cite the filed path', r.stdout, 'the worktree\'s documents')
        r = self.file(src, '13-Papers/02-x-a_1.pdf', res=wt / '95-Resources')
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertTrue((self.res / '13-Papers/02-x-a_1.pdf').is_file(), "filed in the main checkout's folder")
        self.assertEqual([e['file'] for e in self.manifest(wt / '95-Resources')], [ENTRY['file'], '13-Papers/02-x-a_1.pdf'])
        self.assertEqual(self.manifest(), [ENTRY], "the main checkout's manifest waits for the branch")
        self.assertEqual(self.run_script('check').stdout, 'sources: nothing to file\n', "the worktree's entry counts")
        r = self.file(self.place('again.pdf', b'x\n'), '13-Papers/03-y-a_1.pdf')
        self.assertIn('already holds 13-Papers/02-x-a_1.pdf, with these bytes', r.stderr, "another checkout's entry")

    def test_fetch(self):
        self.full()
        served = self.tmp / 'served'
        served.mkdir()
        (served / 'ok.pdf').write_bytes(b'ok\n')
        (served / 'bad.pdf').write_bytes(b'changed\n')
        entries = [ENTRY,
                   dict(ENTRY, file='13-Papers/02-ok-a_1.pdf', download_url=(served / 'ok.pdf').as_uri(), sha256=sha(b'ok\n')),
                   dict(ENTRY, file='13-Papers/03-local-a_1.pdf', sha256=sha(b'local\n')),
                   dict(ENTRY, file='13-Papers/04-local-b_1.pdf', sha256=sha(b'local2\n'))]
        self.write_manifest(self.res, entries)
        self.place('supplied.pdf', b'local2\n')
        (self.inbox / '00-requests.md').write_text('- 2026-11-27 | PR7 | B | u | supplied.pdf\n')
        self.assertIn('00-Inbox/Papers/supplied.pdf: the file of 13-Papers/04-local-b_1.pdf, missing here; 00-sources.py fetch moves '
                      'it into place', self.run_script('check').stdout)
        r = self.run_script('fetch')
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual((self.res / '13-Papers/02-ok-a_1.pdf').read_bytes(), b'ok\n')
        self.assertEqual((self.res / '13-Papers/04-local-b_1.pdf').read_bytes(), b'local2\n')
        self.assertFalse((self.inbox / 'Papers/supplied.pdf').exists())
        self.assertEqual((self.inbox / '00-requests.md').read_text(), '', 'its request line goes')
        self.assertEqual(r.stdout.splitlines(), ['fetched 13-Papers/02-ok-a_1.pdf', 'moved from 00-Inbox: 13-Papers/04-local-b_1.pdf',
                                                 '1 entries without a download URL not fetched (local-only copies; each manifest entry names '
                                                 'its source page): 13-Papers/03-local-a_1.pdf'])
        entries.append(dict(ENTRY, file='13-Papers/05-bad-a_1.pdf', download_url=(served / 'bad.pdf').as_uri(), sha256=sha(b'bad\n')))
        self.write_manifest(self.res, entries)
        (self.res / ENTRY['file']).write_bytes(b'edited\n')
        r = self.run_script('fetch')
        self.assertEqual(r.returncode, 1)
        self.assertIn(f'FAIL {ENTRY["file"]}: local file differs from the manifest SHA-256', r.stdout)
        self.assertIn('FAIL 13-Papers/05-bad-a_1.pdf: downloaded file differs from the manifest SHA-256', r.stdout)
        self.assertFalse((self.res / '13-Papers/05-bad-a_1.pdf').exists() or (self.res / '13-Papers/05-bad-a_1.pdf.part').exists())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')), help='unused: no randomness')
    MODE = parser.parse_args().mode
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(Sources)
    print(f'sources checks: mode={MODE} tests={suite.countTestCases()}', flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
