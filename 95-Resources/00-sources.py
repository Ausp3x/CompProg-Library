#!/usr/bin/env python3
"""Private backup of 95-Resources: the repository CompProg Library Sources (00-index.md, Storage).

A second git history whose working tree is the main checkout's 95-Resources, kept in <git common dir>/sources,
so the public repository never sees it. Maintainer only; the user pushes.

  init           create it, once per clone; restores the files the private repository has and this clone lacks
  status         files not yet saved
  commit [-m M]  save every new or changed file and print the push command
  fetch          download each manifest entry missing here from its download_url and verify its SHA-256
                 (for a contributor's pull request); a failure is listed for the user to supply in 00-Inbox/
"""
import argparse
import datetime
import hashlib
import json
import shlex
import subprocess
import sys
import urllib.request
from pathlib import Path

REMOTE = 'https://github.com/Ausp3x/CompProg-Library-Sources.git'
COMMON = Path(subprocess.run(['git', '-C', str(Path(__file__).resolve().parent), 'rev-parse', '--path-format=absolute',
                              '--git-common-dir'], capture_output=True, text=True, check=True).stdout.strip())
WORK, GITDIR = COMMON.parent / '95-Resources', COMMON / 'sources'


def git(*args, check=True, capture=False):
    r = subprocess.run(['git', f'--git-dir={GITDIR}', f'--work-tree={WORK}', *args], check=check, text=True,
                       capture_output=capture)
    return r.stdout if capture else r.returncode


def init():
    if GITDIR.exists():
        sys.exit(f'{GITDIR} already exists')
    subprocess.run(['git', f'--git-dir={GITDIR}', 'init', '-q', '-b', 'master'], check=True)
    git('config', 'core.bare', 'false')
    git('config', 'core.worktree', str(WORK))
    (GITDIR / 'info' / 'exclude').write_text('/00-Inbox/\n__pycache__/\n*.part\n')
    git('remote', 'add', 'origin', REMOTE)
    if git('ls-remote', '--exit-code', 'origin', 'master', check=False, capture=True):
        git('fetch', '-q', 'origin', 'master')
        git('reset', '-q', 'origin/master')          # index only: local files stay as they are
        git('branch', '-q', '-u', 'origin/master')
        missing = git('ls-files', '-z', '--deleted', capture=True).split('\0')[:-1]
        if missing:
            git('checkout', '--', *missing)
        print(f'restored {len(missing)} files from {REMOTE}')
    else:
        print(f'{REMOTE} has no master branch yet (or no access); run commit, then push')
    print(f'private history at {GITDIR}, working tree {WORK}')


def commit(message):
    git('add', '-A')
    if git('diff', '--cached', '--quiet', check=False) == 0:
        print('nothing to save')
        return
    git('commit', '-q', '-m', message or f'Resources {datetime.date.today()}')
    git('log', '-1', '--stat', '--format=%h %s')
    print('push (the user runs it):\n  git --git-dir=' + shlex.quote(str(GITDIR)) + ' push -u origin master')


def sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        while block := f.read(1 << 20):
            h.update(block)
    return h.hexdigest()


def fetch():
    failed = []
    for e in json.loads((WORK / '99-sources.json').read_text())['resources']:
        path = WORK / e['file']
        if path.exists():
            if sha256(path) != e['sha256']:
                failed.append((e, 'local file differs from the manifest SHA-256'))
            continue
        url = e.get('download_url') or e.get('final_url')
        if not url:
            failed.append((e, 'no download_url in the manifest'))
            continue
        part = path.with_name(path.name + '.part')
        try:
            path.parent.mkdir(parents=True, exist_ok=True)
            req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
            with urllib.request.urlopen(req, timeout=60) as r, open(part, 'wb') as f:
                while block := r.read(1 << 20):
                    f.write(block)
            if sha256(part) != e['sha256']:
                raise ValueError('downloaded file differs from the manifest SHA-256')
            part.rename(path)
            print('fetched', e['file'])
        except Exception as ex:
            part.unlink(missing_ok=True)
            failed.append((e, str(ex)))
    for e, why in failed:
        print(f"FAIL {e['file']}: {why}\n     {e.get('title', '')} <{e.get('download_url') or e.get('source_page', '')}>")
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('command', choices=('init', 'status', 'commit', 'fetch'))
    ap.add_argument('-m', '--message')
    a = ap.parse_args()
    if a.command != 'init' and not GITDIR.exists():
        sys.exit(f'no private history at {GITDIR}; run init first')
    {'init': init, 'status': lambda: git('status', '--short'), 'commit': lambda: commit(a.message),
     'fetch': fetch}[a.command]()
