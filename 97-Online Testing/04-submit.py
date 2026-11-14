#!/usr/bin/env python3
"""Submit expanded Library Checker sources as the maintainer and record each acceptance in its source (06-online.md).

The maintainer runs this, never Claude: `.claude/hooks/shell.py` refuses it for Claude's shell (01-principles.md,
Scope discipline; user decision 2026-11-15).

  04-submit.py login           sign in once: email and password are prompted and never stored; the refresh token goes to
                               ~/.config/compprog/yosupo.json (mode 0600; CP_SUBMIT_CONFIG overrides)
  04-submit.py list            every Yosupo source under src/ with its state: new (no record), current (the recorded
                               submission equals today's bundle), stale (the bundle changed since)
  04-submit.py [SELECT ...]    expand, pick, confirm, submit one at a time, wait for each verdict (green AC, red otherwise
                               on a terminal), record each AC

SELECT: numbers and ranges from `list` (3 5-9), problem names, file names (with or without NN- and .cpp), source paths or globs relative to src/
(`*/03-Polynomials/*`), or the words new, stale, all. With none, the list is shown and the selection is asked for.
Every run expands first (01-expander.py; seconds when nothing changed), so the bytes sent are today's bundle.
An AC rewrites the source's `// Status:` line to
  accepted online (#ID, LANG, DATE UTC, MS ms; the expanded bundle of commit HASH[+dirty], SHA-256 H16); <rest>
where <rest> keeps the old text minus any earlier acceptance or "not verified" clause. States compare bundles with
their Status lines removed, so writing a record does not make a source stale.
"""
from __future__ import annotations

import argparse
import datetime
import fnmatch
import getpass
import hashlib
import importlib.util
import json
import os
import re
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent
LIBRARY = ROOT.parent
SRC, EXPANDED = ROOT / 'src', ROOT / 'expanded'
CONFIG = Path(os.environ.get('CP_SUBMIT_CONFIG', Path.home() / '.config/compprog/yosupo.json'))
# The judge's public Firebase web configuration; the key is read from it at login, never copied into this repository.
FRONTEND_ENV = 'https://raw.githubusercontent.com/yosupo06/library-checker-judge/master/frontend/.env.production'
SIGN_IN = 'https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword'
REFRESH = 'https://securetoken.googleapis.com/v1/token'
PROBLEM = re.compile(r'^// PROBLEM: https://judge\.yosupo\.jp/problem/(\w+)\s*$', re.M)
STATUS = re.compile(r'^// Status: ?(.*)$', re.M)
RECORD = re.compile(r'accepted online \(#(\d+)')
POLL, WAIT = 2.0, 1800.0   # seconds between status checks; give up on one submission after this long
TOKEN_AGE = 2700.0         # seconds before a fresh ID token is fetched (Firebase's expire after an hour)


def judge_module():
    spec = importlib.util.spec_from_file_location('judge', LIBRARY / '96-Local Testing/00-Tools/07-judge.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


J = judge_module()


def request(url, data=None, form=False, token=None):
    """JSON of a GET (data None) or POST; HTTP errors become SystemExit with the server's message."""
    headers = {'Authorization': f'Bearer {token}'} if token else {}
    body = None
    if data is not None:
        body = (urllib.parse.urlencode(data) if form else json.dumps(data)).encode()
        headers['Content-Type'] = 'application/x-www-form-urlencoded' if form else 'application/json'
    try:
        with urllib.request.urlopen(urllib.request.Request(url, body, headers), timeout=60) as r:
            text = r.read().decode()
    except urllib.error.HTTPError as e:
        raise SystemExit(f'{url.split("?")[0]}: HTTP {e.code}: {e.read().decode(errors="replace")[:500]}')
    except OSError as e:
        raise SystemExit(f'{url.split("?")[0]}: {e}')
    return json.loads(text) if text else {}


def api_key():
    text = urllib.request.urlopen(FRONTEND_ENV, timeout=60).read().decode()
    m = re.search(r'^VITE_FIREBASE_API_KEY=(\S+)$', text, re.M)
    if not m:
        raise SystemExit(f'no VITE_FIREBASE_API_KEY in {FRONTEND_ENV}')
    return m.group(1)


def save(config):
    CONFIG.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    tmp = CONFIG.with_suffix('.tmp')
    fd = os.open(tmp, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(fd, 'w') as f:
        json.dump(config, f)
    os.replace(tmp, CONFIG)


def whoami(token):
    user = request(J.API + '/auth/current_user', token=token).get('user')
    if not user:
        raise SystemExit('the judge knows no user for this login: register the account on judge.yosupo.jp first')
    return user['name']


def login(ask=input, secret=getpass.getpass):
    key = api_key()
    r = request(f'{SIGN_IN}?key={key}', {'email': ask('Library Checker email: ').strip(), 'password': secret('Password: '),
                                         'returnSecureToken': True})
    name = whoami(r['idToken'])
    save({'api_key': key, 'refresh_token': r['refreshToken'], 'user': name})
    print(f'logged in as {name}; refresh token saved to {CONFIG}')


def id_token():
    """A fresh ID token from the saved refresh token, checked to still name the saved user."""
    if not CONFIG.exists():
        raise SystemExit(f'no login at {CONFIG}: run `04-submit.py login` first (anonymous submissions are refused)')
    config = json.loads(CONFIG.read_text())
    r = request(f'{REFRESH}?key={config["api_key"]}', {'grant_type': 'refresh_token', 'refresh_token': config['refresh_token']}, form=True)
    if r.get('refresh_token') and r['refresh_token'] != config['refresh_token']:
        save({**config, 'refresh_token': r['refresh_token']})
    name = whoami(r['id_token'])
    if name != config['user']:
        raise SystemExit(f'the saved login now names {name}, not {config["user"]}: run `04-submit.py login` again')
    return r['id_token'], name


def strip_status(text):
    return '\n'.join(line.rstrip() for line in text.replace('\r', '').strip().split('\n') if not line.startswith('// Status:'))


def sources():
    """[(src-relative path, problem, source path, bundle path)] for every Yosupo source, in path order."""
    out = []
    for path in sorted(SRC.rglob('*.cpp')):
        if path.name.endswith('.expanded.cpp'):
            continue
        m = PROBLEM.search(path.read_text())
        if m:
            rel = path.relative_to(SRC).as_posix()
            out.append((rel, m.group(1), path, EXPANDED / rel.replace('.cpp', '.expanded.cpp')))
    return out


def state(path, bundle):
    """(new | current | stale, recorded submission id or None); current means the judge holds this bundle."""
    status = STATUS.search(path.read_text())
    m = RECORD.search(status.group(1)) if status else None
    if not m:
        return 'new', None
    try:
        _, held = J.submission(m.group(1))
    except OSError as e:
        raise SystemExit(f'#{m.group(1)}, recorded in {path.name}: {e}')
    same = bundle.exists() and strip_status(held.read_text()) == strip_status(bundle.read_text())
    return ('current' if same else 'stale'), m.group(1)


def pick(words, rows, states):
    """Indices into rows chosen by words (numbers 1-based as listed; a file name matches with or without its NN- prefix and .cpp), in the order given, without repeats."""
    chosen = []
    for w in words:
        if re.fullmatch(r'\d+(-\d+)?', w):
            a, _, b = w.partition('-')
            hits = [i - 1 for i in range(int(a), int(b or a) + 1)]
            if not hits or min(hits) < 0 or max(hits) >= len(rows):
                raise SystemExit(f'{w}: the list has entries 1 to {len(rows)}')
        elif w in ('all', 'new', 'stale'):
            hits = [i for i in range(len(rows)) if w == 'all' or states[i][0] == w]
        else:
            hits = [i for i, (rel, prob, path, _) in enumerate(rows)
                    if w in (prob, path.name, path.stem, path.stem.split('-', 1)[-1]) or fnmatch.fnmatch(rel, w) or w.rstrip('/') in (rel, f'src/{rel}')]
            if not hits:
                raise SystemExit(f'{w}: no source, file name, problem or glob matches')
        chosen += [i for i in hits if i not in chosen]
    return chosen


def show(rows, states):
    for i, ((rel, prob, _, _), (st, sid)) in enumerate(zip(rows, states), 1):
        print(f'{i:3}  {st:7}  {prob:50}  {rel}' + (f'  (#{sid})' if sid else ''))


def commit(paths):
    """HEAD's short hash, with +dirty when any of paths (the source and the library folders) has uncommitted changes."""
    head = subprocess.run(['git', '-C', str(LIBRARY), 'rev-parse', '--short', 'HEAD'], capture_output=True, text=True).stdout.strip()
    dirty = subprocess.run(['git', '-C', str(LIBRARY), 'status', '--porcelain', '--', *map(str, paths)], capture_output=True, text=True).stdout
    return head + ('+dirty' if dirty.strip() else '')


def record(path, sid, lang, overview, sent, rev):
    """Write the acceptance into path's Status line (inserted after the PROBLEM line when there is none)."""
    day = overview['submission_time'][:10] if overview.get('submission_time') else datetime.datetime.now(datetime.UTC).date().isoformat()
    ms = round(float(overview.get('time') or 0) * 1000)
    acc = f'accepted online (#{sid}, {lang}, {day} UTC, {ms} ms; the expanded bundle of commit {rev}, SHA-256 {hashlib.sha256(sent).hexdigest()[:16]})'
    text = path.read_text()
    status = STATUS.search(text)
    if status:
        rest = re.sub(r'(^|;\s*)accepted online \(#\d+[^)]*\)', '', status.group(1))
        rest = re.sub(r';?\s*online acceptance not verified[^;]*', '', rest).strip(' ;.')
        text = text[:status.start()] + f'// Status: {acc}' + (f'; {rest}.' if rest else '.') + text[status.end():]
    else:
        m = PROBLEM.search(text)
        text = text[:m.end()] + f'\n// Status: {acc}.' + text[m.end():]
    path.write_text(text)


def paint(text, ok):
    """text in green (ok) or red on a terminal, plain when redirected."""
    return f'\033[{32 if ok else 31}m{text}\033[0m' if sys.stdout.isatty() else text


def wait(sid):
    """The submission's details once judged; a network error is retried like a pending status until WAIT runs out."""
    start = time.monotonic()
    while True:
        try:
            d, _ = J.submission(sid)
            status = d['overview']['status']
            if not J.pending(status):
                return d
        except OSError as e:
            status = f'unreachable ({e})'
        if time.monotonic() - start > WAIT:
            raise SystemExit(f'#{sid} still {status} after {WAIT:.0f} s; check https://judge.yosupo.jp/submission/{sid}')
        time.sleep(POLL)


def expand():
    r = subprocess.run([sys.executable, str(ROOT / '01-expander.py')], capture_output=True, text=True)
    if r.returncode:
        raise SystemExit(f'01-expander.py failed; nothing submitted:\n{(r.stdout + r.stderr)[-2000:]}')


def submit(words, lang='cpp', yes=False, ask=input):
    expand()
    rows = sources()
    states = [state(p, b) for _, _, p, b in rows]
    if not words:
        show(rows, states)
        words = ask('Submit which (numbers, ranges, problems, file names, globs, new, stale, all; empty to quit)? ').split()
        if not words:
            return 0
    chosen = pick(words, rows, states)
    for i in chosen:
        print(f'  {rows[i][1]:50}  {rows[i][0]}  [{states[i][0]}]')
    if not chosen or not (yes or ask(f'Submit these {len(chosen)} as {lang}? [y/N] ').strip().lower() in ('y', 'yes')):
        print('nothing submitted')
        return 0
    folders = [p for p in LIBRARY.glob('0[1-8]-*') if p.is_dir()]
    token, fresh, failed = None, 0.0, 0
    for i in chosen:
        rel, prob, path, bundle = rows[i]
        if token is None or time.monotonic() - fresh > TOKEN_AGE:
            token, name = id_token()
            fresh = time.monotonic()
        sent = bundle.read_bytes()
        rev = commit([path, *folders])
        sid = request(J.API + '/submit', {'problem': prob, 'source': sent.decode(), 'lang': lang}, token=token)['id']
        print(f'{prob}: #{sid} as {name}, judging ...', flush=True)
        o = wait(sid)['overview']
        print(f'{prob}: #{sid} {paint(o["status"], o["status"] == "AC")} {round(float(o.get("time") or 0) * 1000)} ms  https://judge.yosupo.jp/submission/{sid}')
        if o['status'] == 'AC':
            record(path, sid, lang, o, sent, rev)
        else:
            failed += 1
    accepted = len(chosen) - failed
    print(paint(f'{accepted} accepted, {failed} not', not failed) + ('; AC records written to the sources\' Status lines (commit them).' if accepted else '.'))
    return failed


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('select', nargs='*', help='login, list, or what to submit (see above)')
    parser.add_argument('--lang', default='cpp', help='judge language id (default cpp: C++23)')
    parser.add_argument('--yes', action='store_true', help='skip the confirmation')
    a = parser.parse_args(argv)
    if a.select == ['login']:
        login()
        return 0
    if a.select == ['list']:
        expand()
        rows = sources()
        show(rows, [state(p, b) for _, _, p, b in rows])
        return 0
    return 1 if submit(a.select, a.lang, a.yes) else 0


if __name__ == '__main__':
    sys.exit(main())
