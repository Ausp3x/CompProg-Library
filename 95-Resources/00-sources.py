#!/usr/bin/env python3
"""Private backup of 95-Resources: the repository CompProg Library Sources (00-index.md, Storage).

A second git history whose working tree is the main checkout's 95-Resources, kept in <git common dir>/sources,
so the public repository never sees it. Maintainer only; the user pushes.

  init           create it, once per clone; restores the files the private repository has and this clone lacks
  status         files not yet saved
  commit [-m M]  save every new or changed file and print the push command; from a worktree it first copies the
                 manifest files the main checkout lacks there (/finish runs it)
  fetch          bring each manifest entry missing here: a file in 00-Inbox/ with its SHA-256 is moved into place,
                 otherwise it is downloaded from its download_url and its SHA-256 verified; needs no private history (a
                 contributor's setup, CONTRIBUTING.md); a failure is listed for the user to supply in 00-Inbox/, and the
                 local-only entries without a URL are named in one line, not failed
  file SRC DEST --title T --url U --cited-in DOC [--cited-in DOC ...] [--download-url D] [--pages N] [--provided-by P]
       [--reuse-note R]
                 file a source the work uses (00-index.md, Storage): move SRC (an inbox file or the session's own
                 download) to DEST, 13-Papers/<name> or 14-Web Pages/<name> of the main checkout, add its manifest
                 entry to this checkout's 99-sources.json with the documents citing it (paths from the checkout's root,
                 no line numbers), delete the request lines naming it, and with a private history commit the file
                 there at once
  check [ID ...] what Storage still requires, one line each, exit 1 if any (gaps.py prints it for /package and
                 /finish): inbox files placed for these package or batch ids, with no request line, or copies of filed
                 ones; Markdown citing a file in 00-Inbox/; manifest entries whose cited_in names a document missing here
                 or not naming the file; filed files without a manifest entry; with a private history, manifest files
                 missing here and filed files not backed up
"""
import argparse
import datetime
import hashlib
import json
import re
import shlex
import shutil
import subprocess
import sys
import urllib.request
from pathlib import Path

REMOTE = 'https://github.com/Ausp3x/CompProg-Library-Sources.git'
HERE = Path(__file__).resolve().parent
COMMON = Path(subprocess.run(['git', '-C', str(HERE), 'rev-parse', '--path-format=absolute',
                              '--git-common-dir'], capture_output=True, text=True, check=True).stdout.strip())
WORK, GITDIR = COMMON.parent / '95-Resources', COMMON / 'sources'
INBOX = WORK / '00-Inbox'
REQUESTS = INBOX / '00-requests.md'  # one line per open request: - YYYY-MM-DD | package or batch | title | URL | file name
FOLDERS = ('13-Papers', '14-Web Pages')  # where every source the work uses is filed (00-index.md, Storage)
NAME = re.compile(r'\d\d-[a-z0-9_]+(-[a-z0-9_]+)*\.[a-z0-9]+')  # NN-short_title-author_year, NN-site_id-short_title
CITE = re.compile(r'00-Inbox/(?!00-requests\.md)[^\s`|)<>]')  # a path into the inbox, where the filed path belongs
LOCAL = 'Personal study copy; terms do not permit public redistribution, so the file stays local (git-ignored).'


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
    if HERE != WORK:  # a worktree: its git-ignored sources would vanish with it, so copy the new ones to the main checkout
        for e in json.loads((HERE / '99-sources.json').read_text())['resources']:
            src, dst = HERE / e['file'], WORK / e['file']
            if src.exists() and not dst.exists():
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, dst)
                print('copied from the worktree:', e['file'])
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
    failed, local = [], []
    placed = {sha256(f): f for f in INBOX.rglob('*') if f.is_file() and f != REQUESTS} if INBOX.is_dir() else {}
    for e in json.loads((WORK / '99-sources.json').read_text())['resources']:
        path = WORK / e['file']
        if path.exists():
            if sha256(path) != e['sha256']:
                failed.append((e, 'local file differs from the manifest SHA-256'))
            continue
        if src := placed.pop(e['sha256'], None):  # supplied by the user for an entry the download could not bring
            path.parent.mkdir(parents=True, exist_ok=True)
            shutil.move(src, path)
            drop_request(src.name)
            print('moved from 00-Inbox:', e['file'])
            continue
        url = e.get('download_url') or e.get('final_url')
        if not url:  # a local-only copy (a saved page, a paper without a direct download): only the maintainer holds it
            local.append(e['file'])
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
    if local:
        print(f'not fetched, having no download URL (local-only copies; each manifest entry names its source page): '
              f'{", ".join(local)}')
    for e, why in failed:
        print(f"FAIL {e['file']}: {why}\n     {e.get('title', '')} <{e.get('download_url') or e.get('source_page', '')}>")
    sys.exit(1 if failed else 0)


def requests():
    """(line, package or batch, file name) of each request line in 00-Inbox/00-requests.md."""
    out = []
    for line in REQUESTS.read_text().splitlines() if REQUESTS.is_file() else []:
        parts = line[2:].split(' | ') if line.startswith('- ') else []
        if len(parts) >= 5 and re.fullmatch(r'\d{4}-\d\d-\d\d', parts[0]):
            out.append((line, parts[1].strip(), parts[-1].strip()))
    return out


def drop_request(name):
    """Delete the request lines naming the file `name`, which is filed."""
    if gone := {line for line, _, n in requests() if n == name}:
        REQUESTS.write_text(''.join(line + '\n' for line in REQUESTS.read_text().splitlines() if line not in gone))


def manifests():
    """Manifest entries by file over every checkout of this clone: a worktree's new entries reach master with it."""
    listed = subprocess.run(['git', '-C', str(HERE), 'worktree', 'list', '--porcelain'], capture_output=True, text=True).stdout
    entries = {}
    for root in [l.split(' ', 1)[1] for l in listed.splitlines() if l.startswith('worktree ')] or [str(HERE.parent)]:
        manifest = Path(root) / '95-Resources/99-sources.json'
        for e in json.loads(manifest.read_text())['resources'] if manifest.is_file() else []:
            entries.setdefault(e['file'], e)
    return entries


def citations(root):
    """'doc:line' of each line outside code fences, in the checkout's tracked or new Markdown outside OLD and
    95-Resources, that cites a file in 00-Inbox (03-consistency.py rejects them)."""
    docs = subprocess.run(['git', '-C', str(root), 'ls-files', '-z', '--cached', '--others', '--exclude-standard', '--', '*.md'],
                          capture_output=True, text=True).stdout.split('\0')
    out = []
    for doc in sorted(set(docs)):
        if not doc or doc.startswith(('OLD/', '95-Resources/')) or not (Path(root) / doc).is_file():
            continue
        fence = False
        for i, line in enumerate((Path(root) / doc).read_text(errors='replace').splitlines(), 1):
            fence ^= bool(re.match(r'\s*(```|~~~)', line))
            if not fence and CITE.search(line):
                out.append(f'{doc}:{i}')
    return out


def drifted(root):
    """Each manifest entry of the checkout whose cited_in does not list documents that exist there and name its file
    (03-consistency.py rejects them); the 2026-09-27 downloads outside FOLDERS record their use instead."""
    manifest, out = Path(root) / '95-Resources/99-sources.json', []
    for e in json.loads(manifest.read_text())['resources'] if manifest.is_file() else []:
        docs = e.get('cited_in')
        if docs is None and not e['file'].startswith(FOLDERS):
            continue
        if not (isinstance(docs, list) and docs and all(isinstance(d, str) for d in docs)):
            out.append(f"{e['file']}: cited_in must list the documents citing it")
            continue
        for doc in docs:
            p = Path(root) / doc
            if not (p.is_file() and Path(e['file']).name in p.read_text(errors='replace')):
                out.append(f"{e['file']}: cited_in names {doc}, which " + ('does not name the file' if p.is_file() else 'does not exist'))
    return out


def file_(a):
    src = Path(a.source).resolve()
    folder, _, name = a.dest.partition('/')
    if folder not in FOLDERS or not NAME.fullmatch(name):
        sys.exit(f'file: name the destination 13-Papers/<name> or 14-Web Pages/<name>, the name lowercase '
                 f'NN-short_title-author_year for a paper or NN-site_id-short_title for a page (00-index.md): {a.dest}')
    if not src.is_file():
        sys.exit(f'file: no file {src}')
    if bad := [d for d in a.cited_in if not (HERE.parent / d).is_file()]:
        sys.exit(f'file: --cited-in names a document of this checkout by its path from the root, without a line number: {", ".join(bad)}')
    digest, known = sha256(src), manifests()
    if clash := next((f for f, e in known.items() if f == a.dest or e['sha256'] == digest), None):
        sys.exit(f'file: the manifest already holds {clash}' + (', with these bytes: cite it and delete this copy'
                                                               if known[clash]['sha256'] == digest else ''))
    target = WORK / a.dest
    if target.exists():
        sys.exit(f'file: {target} exists without a manifest entry; add its entry or choose another name')
    inbox = src.is_relative_to(INBOX.resolve())
    entry = {'file': a.dest, 'title': a.title, 'source_page': a.url, **({'download_url': a.download_url} if a.download_url else {}),
             'downloaded': str(datetime.date.today()), 'bytes': src.stat().st_size, **({'pages': a.pages} if a.pages else {}),
             'sha256': digest, 'cited_in': a.cited_in, 'provided_by': a.provided_by or ('user' if inbox else 'session'),
             'reuse_note': a.reuse_note or LOCAL, 'committed': False}
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.move(src, target)
    manifest = HERE / '99-sources.json'
    data = json.loads(manifest.read_text())
    data['resources'].append(entry)
    manifest.write_text(json.dumps(data, indent=2, ensure_ascii=False) + '\n')
    if inbox:
        drop_request(src.name)
    print(f'filed {a.dest} (SHA-256 {digest}); cite it by that path in {", ".join(a.cited_in)}')
    if GITDIR.exists():  # backed up at once, never left for a later step (00-index.md, Storage)
        git('add', '--', a.dest)
        git('commit', '-q', '-m', f'File {a.dest}')
        print('backed up in the private repository; push (the user runs it):\n  git --git-dir=' + shlex.quote(str(GITDIR))
              + ' push -u origin master')


def problems(keys=()):
    """What 00-index.md (Storage) still requires, one line each; files placed for `keys` (package or batch ids) count
    as this work's, and files another session requested stay until it files them."""
    out, known = [], manifests()
    filed = {e['sha256']: f for f, e in known.items()}
    owner = {n: k for _, k, n in requests()}
    for f in sorted(p for p in INBOX.rglob('*') if p.is_file() and p != REQUESTS) if INBOX.is_dir() else []:
        rel, who = f.relative_to(WORK), owner.get(f.name)
        if same := filed.get(sha256(f)):
            out.append(f'{rel}: a copy of the filed {same}; delete it' if (WORK / same).exists() else
                       f'{rel}: the file of {same}, missing here; 00-sources.py fetch moves it into place')
        elif who is None:
            out.append(f'{rel}: no request line; file it if this work used it (00-sources.py file), else ask the user')
        elif who in keys:
            out.append(f'{rel}: placed for {who}; file it (00-sources.py file)')
    out += [f'{hit}: cites a file in 00-Inbox; file it and cite the filed path' for hit in citations(HERE.parent)]
    out += drifted(HERE.parent)
    for folder in FOLDERS:
        found = sorted(p for p in (WORK / folder).rglob('*') if p.is_file()) if (WORK / folder).is_dir() else []
        out += [f'{rel}: no manifest entry' for f in found if (rel := f.relative_to(WORK).as_posix()) not in known]
    if GITDIR.exists():
        out += [f'{f}: in the manifest, missing here' for f in known if f.startswith(FOLDERS) and not (WORK / f).exists()]
        if unsaved := [line for line in git('status', '--short', '--untracked-files=all', '--', *FOLDERS, capture=True).splitlines() if line]:
            out.append(f'filed files not backed up ({len(unsaved)}): 00-sources.py commit')
    return out


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('command', choices=('init', 'status', 'commit', 'fetch', 'file', 'check'))
    ap.add_argument('args', nargs='*', help='file: SRC DEST; check: package or batch ids')
    ap.add_argument('-m', '--message')
    for flag in ('--title', '--url', '--download-url', '--provided-by', '--reuse-note'):
        ap.add_argument(flag)
    ap.add_argument('--cited-in', action='append', help='file: a document citing the source; repeat for each')
    ap.add_argument('--pages', type=int)
    a = ap.parse_args()
    if a.command in ('status', 'commit') and not GITDIR.exists():
        sys.exit(f'no private history at {GITDIR}; run init first')
    if a.command == 'file':
        if len(a.args) != 2 or not (a.title and a.url and a.cited_in):
            ap.error('file needs SRC DEST, --title, --url (the page or DOI the evidence cites) and --cited-in DOC for each citing document')
        a.source, a.dest = a.args
        file_(a)
    elif a.command == 'check':
        found = problems(set(a.args))
        print('\n'.join(found) or 'sources: nothing to file')
        sys.exit(bool(found))
    else:
        {'init': init, 'status': lambda: git('status', '--short'), 'commit': lambda: commit(a.message),
         'fetch': fetch}[a.command]()
