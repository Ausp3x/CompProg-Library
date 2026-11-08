#!/usr/bin/env python3
"""Maintainer gate for a contributor's pull request (/merge-pr). Run from the main checkout with master checked out.

Usage: merge_pr.py files N | checkout N | gates N | run N [--mb MB] -- CMD... | land N | drop N
  files     fetch the pull request head as data (nothing runs) and classify every changed path as library code or
            outside it; an outside path needs the user's yes before checkout, and so does each added line that records
            a decision (DECISIONS), which the user confirms is theirs.
  checkout  add the worktree .claude/worktrees/PR<N> on the head branch and rebase it with worktree.py --sync.
  gates     run the gates (consistency, plan, the package's suites, the mutation records of changed headers, integration,
            workspace) in that worktree, each in bwrap (system read-only, home and /run hidden, no network, the
            worktree seen through a throwaway overlay, a private /tmp) inside a memory-capped systemd scope; logs in
            .claude/merge-pr/<N>/. `plan doctor` fails only on a line master's own doctor does not report.
  run       one command sandboxed and capped the same way, for @reviewer and for checks of fixes.
  land      once the gates passed on the current head over the current master and the pull request still has that
            head: worktree.py --finish (fast-forward master, remove the worktree), then print the push and close
            commands for the user. Never pushes.
  drop      remove the worktree, the branch checkout created and the fetched head without landing.
"""
import argparse
import contextlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[3]
STATE = ROOT / '.claude/merge-pr'
TESTS = '96-Local Testing/'
# Library code: algorithm folders, their testers and judge solutions, the notebook selection and site articles, and the
# records a package updates (user decision 2026-11-01).
LIBRARY = re.compile(r'0[1-8]-[^/]+/.+|96-Local Testing/(0[1-8]-[^/]+/.+|00-index\.md)|97-Online Testing/(src|Docs)/.+'
                     r'|98-Team Notebook/(02-selection\.json|Articles/.+)'
                     r'|00-Guidelines/(Ledgers|23-Sweep Ledgers)/[^/]+|00-Guidelines/13-Plan/(packages\.json|finish-log\.tsv)')
# path components outside library code in any folder: git and agent files, and bytecode or extension modules, which
# Python runs in place of the reviewed source
HIDDEN = re.compile(r'\..*|CLAUDE.*\.md|AGENTS\.md|__pycache__|.*\.(pyc|so)')
# Outside paths checkout's --sync runs on the host: 03-consistency.py and what it loads, plan.py rank and render, and any
# module or package beside them, since their folders are on the import path and can shadow standard modules ...
HOST_RUN = re.compile(r'(96-Local Testing|00-Guidelines/13-Plan)/([^/]+\.(py|pyc|so)|[A-Za-z_]\w*/.+)'
                      r'|98-Team Notebook/(01-notebook|05-dependencies)\.py|\.claude/skills/setup-check/setup_check\.py')
AGENT = re.compile(r'(.+/)?\.claude/.+|(.+/)?(CLAUDE[^/]*\.md|AGENTS\.md)|\.mcp\.json')  # ... and those Claude Code loads in the worktree
HEADER = re.compile(r'0[1-8]-[^/]+/(\d{2}-[^/]+\.hpp|_\d{2}_[^/]+\.py)')  # an inventory target
MUTATED = re.compile(r'0[1-7]-[^/]+/(?!9[89]-)\d{2}-[^/]+\.hpp')  # 04-mutation.py HEADER (10-merge_tester compares them)
MECHANICAL = re.compile(r'P\d{3}: (prune duplicates|restyle)')  # the commit subjects of /prune and /restyle
# a line that records someone's decision: the user's, or one dated (`decided 2026-11-05`, `(user, 2026-11-05)`)
DECISION = re.compile(r'\buser\b.{0,40}\bdeci(?:d|sion)|\bdeci(?:d|sion)\w*.{0,40}\buser\b|\bdecided\s+\d{4}-\d{2}-\d{2}|\(user,?\s+\d{4}-', re.I)
CORE_FULL = re.compile(r'01-Core/\d{2}-(?![^/]*mini)[^/]*\.hpp')
ENV = ('PATH', 'HOME', 'USER', 'LANG', 'LC_ALL', 'TERM', 'XDG_RUNTIME_DIR', 'DBUS_SESSION_BUS_ADDRESS')  # the rest stays outside
HIDE = ('/mnt', '/media', '/srv')  # mounts hidden inside the sandbox besides /run and the home directory
TIMEOUT = 7200  # seconds per gate
MAX_MB = 16384  # ceiling on a cap an entry point declares with ensure(mb=...)


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


plan = module('merge_plan', ROOT / '00-Guidelines/13-Plan/plan.py')
cap = module('merge_memory_cap', ROOT / TESTS / '_00_memory_cap.py')


def git(*args, cwd=ROOT, check=True):
    r = subprocess.run(['git', *args], cwd=cwd, capture_output=True, text=True)
    if check and r.returncode:
        raise SystemExit(f'git {" ".join(args)} failed:\n{r.stderr.strip()}')
    return r.stdout.rstrip() if check else r


def outside_block(rev):
    """01-prompts.md at rev without its generated plan block."""
    text = git('show', f'{rev}:01-prompts.md', check=False).stdout
    return text[:text.index(plan.BEGIN)] + text[text.index(plan.END):] if plan.BEGIN in text and plan.END in text else text


def changes(base, head):
    """(status, path, why the path is outside library code, or '') for every path changed from base to head."""
    out, res = git('diff', '--raw', '-z', '--no-renames', base, head).split('\0'), []
    for meta, path in zip(out[0::2], out[1::2]):
        old, new, *_, status = meta[1:].split(' ')
        modes = {old, new}
        library = not any(HIDDEN.fullmatch(c) for c in path.split('/')) and (
            LIBRARY.fullmatch(path) or path == '01-prompts.md' and outside_block(base) == outside_block(head))
        why = ('symlink' if '120000' in modes else 'submodule' if '160000' in modes else '' if library else 'outside library code'
               + ('; runs on the host at checkout' if HOST_RUN.fullmatch(path) else '; loaded by Claude Code' if AGENT.fullmatch(path) else ''))
        res.append((status, path, why))
    return res


def decisions(base, head):
    """(path, line, text) of every line added from base to head that records a decision (DECISION) or sits under the
    `## Recorded decisions` heading of a sweep ledger (headings aside); the generated 01-prompts.md is skipped, its
    notes come from packages.json."""
    out, path, n, texts = [], None, 0, {}
    diff = git('-c', 'core.quotePath=false', 'diff', '-U0', '--no-color', '--no-ext-diff', '--no-renames', base, head, '--', '.', ':(exclude)01-prompts.md')
    for line in diff.split('\n'):
        if line.startswith('+++ '):
            path = line[6:].removesuffix('\t') if line.startswith('+++ b/') else None  # git tabs a path with spaces
        elif line.startswith('@@ '):
            n = int(re.match(r'@@ -\S+ \+(\d+)', line)[1])
        elif line.startswith('+') and path:
            if path.startswith('00-Guidelines/23-Sweep Ledgers/') and path not in texts:
                texts[path] = git('show', f'{head}:{path}').split('\n')
            heading = next((h for h in reversed(texts.get(path, [])[:n - 1]) if h.startswith('## ')), '')
            if line[1:].strip() and (DECISION.search(line) or heading.strip() == '## Recorded decisions' and not line.startswith('+#')):
                out.append((path, n, line[1:]))
            n += 1
    return out


def state(n):
    p = STATE / f'{n}.json'
    if not p.is_file():
        raise SystemExit(f'no record of #{n}: run `merge_pr.py files {n}` first')
    return json.loads(p.read_text())


def checked_out(n):
    st = state(n)
    if not Path(st['worktree']).is_dir():
        raise SystemExit(f'#{n} is not checked out: run `merge_pr.py checkout {n}` first')
    return st, Path(st['worktree'])


def save(n, st):
    STATE.mkdir(parents=True, exist_ok=True)
    (STATE / f'{n}.json').write_text(json.dumps(st, indent=1) + '\n')


def view(n, repo, fields):
    out, why = plan.gh(ROOT, 'pr', 'view', str(n), '--repo', repo, '--json', fields)
    if out is None:
        raise SystemExit(f'gh pr view {n}: {why}')
    return json.loads(out)


def files(n):
    if plan.role(ROOT) != 'maintainer':
        raise SystemExit('/merge-pr is for the maintainer (git config compprog.role maintainer)')
    old = state(n) if (STATE / f'{n}.json').is_file() else {}
    if old and Path(old['worktree']).exists():
        raise SystemExit(f'#{n} is checked out at {old["worktree"]}: land or drop it first')
    repo = plan.config(ROOT, 'compprog.repo') or plan.REPO
    pr = view(n, repo, 'number,title,state,isDraft,baseRefName,headRefName,headRefOid,author')
    if pr['state'] != 'OPEN' or pr['baseRefName'] != 'master':
        raise SystemExit(f'STOP: #{n} is {pr["state"].lower()} against {pr["baseRefName"]}; only open pull requests into master land')
    if pr['isDraft']:
        raise SystemExit(f'STOP: #{n} is a draft, so its package is not done; the contributor marks it ready after the last batch')
    ref = f'refs/merge-pr/{n}'
    git('fetch', '--no-tags', '--quiet', f'https://github.com/{repo}.git', f'+refs/pull/{n}/head:{ref}')
    head = git('rev-parse', ref)
    if head != pr['headRefOid']:
        raise SystemExit(f'#{n} moved while it was fetched; rerun')
    base = git('merge-base', 'master', head)
    _, packages = plan.load(ROOT)
    m = plan.CLAIM.fullmatch(pr['headRefName'])
    pkg = next((p for p in packages['packages'] if m and p['id'] == m[1] and m[2].upper() in p['batches']), None)
    rows = changes(base, head)
    core = [p for _, p, _ in rows if CORE_FULL.fullmatch(p)]
    model = 'fable' if core else pkg and pkg['model']
    save(n, dict(repo=repo, head=head, base=base, branch=pr['headRefName'] if pkg else f'pr-{n}', package=pkg and pkg['id'],
                 model=model, worktree=str(ROOT / '.claude/worktrees' / f'PR{n}'), **{k: v for k, v in old.items() if k == 'created'}))
    commits = git('rev-list', '--count', f'{base}..{head}')
    print(f'#{n} "{pr["title"]}" by @{pr["author"]["login"]} on {pr["headRefName"]}: '
          + (f'package {pkg["id"]} ({pkg["model"]}), batch {m[2].upper()}' if pkg else 'not a claim branch, so no package'))
    print(f'head {head[:12]}, {commits} commit(s) on merge base {base[:12]}; {len(rows)} path(s) changed; reviewer model {model or "default"}')
    if core:
        print(f'CORE FULL: {", ".join(core)}; Core full packages are maintainer-only, so a contributor\'s change to them needs the user\'s yes')
    folders = {}
    for status, path, why in rows:
        if why:
            print(f'  OUTSIDE {status} {path} ({why})')
        else:
            folders[path.split('/')[0]] = folders.get(path.split('/')[0], 0) + 1
    print('  library: ' + (', '.join(f'{f} {k}' for f, k in sorted(folders.items())) or 'none'))
    out = [r for r in rows if r[2]]
    print(f'OUTSIDE LIBRARY: {len(out)} of {len(rows)} paths; the user must say yes before checkout' if out else 'all paths are library code')
    dec = decisions(base, head)
    if dec:
        print(f'DECISIONS: {len(dec)} added line(s) record a decision; the user confirms each is theirs before checkout')
        print('\n'.join(f'  {path}:{n}: {text.strip()[:200]}' for path, n, text in dec))
    return 0


def checkout(n):
    st = state(n)
    wt = Path(st['worktree'])
    if wt.exists():
        raise SystemExit(f'{wt} exists; `merge_pr.py drop {n}` removes it')
    git('worktree', 'add', '--quiet', '-b', st['branch'], str(wt), st['head'])  # fails on an existing branch, which stays the user's
    save(n, st | dict(created=True))
    r = subprocess.run([sys.executable, str(ROOT / plan.PLAN / 'worktree.py'), str(wt), '--sync'], cwd=ROOT, capture_output=True, text=True)
    # worktree.py reports "not merged into master" as NOT DONE on stdout; any stop (conflict, new validator errors) is on stderr
    if r.stderr.strip() or git('branch', '--show-current', cwd=wt) != st['branch'] or git('merge-base', '--is-ancestor', 'master', 'HEAD', cwd=wt, check=False).returncode:
        print(r.stdout + r.stderr, end='')
        return 1
    print(''.join(l + '\n' for l in r.stdout.splitlines() if l.startswith(('sync', 'note'))) + f'{wt} is based on master; next: gates')
    return 0


def sandbox(cmd, wt, tmp):
    """cmd under bwrap: / read-only; /run, /mnt, /media, /srv and the home directory hidden, its PATH directories, the
    directory of the mewt that PATH links to, and the repository's git directory back read-only with the other worktrees' git directories hidden; no network, IPC,
    PID or UTS sharing; the worktree through an overlay whose writes land in `tmp`/upper, and `tmp`/tmp as /tmp. So
    nothing the command writes reaches the real worktree, where host code (the next --sync) would use it."""
    home = str(Path.home())
    common = git('rev-parse', '--path-format=absolute', '--git-common-dir', cwd=wt)
    own = git('rev-parse', '--absolute-git-dir', cwd=wt)
    keep = [d for d in os.environ.get('PATH', '').split(':') if d.startswith(home + '/') and Path(d).is_dir()]
    tool = shutil.which('mewt')  # installed as a link into PREFIX/opt, which would dangle under the hidden home
    if tool and str(Path(tool).resolve().parent).startswith(home + '/'):
        keep.append(str(Path(tool).resolve().parent))
    res = ['bwrap', '--ro-bind', '/', '/', '--dev', '/dev', '--proc', '/proc', '--tmpfs', '/run', '--tmpfs', home]
    res += [x for d in HIDE if Path(d).is_dir() for x in ('--tmpfs', d)] + ['--bind', str(tmp / 'tmp'), '/tmp']
    for d in [*keep, common]:
        res += ['--ro-bind', d, d]
    res += ['--tmpfs', str(Path(common) / 'worktrees'), '--ro-bind', own, own] if Path(own).parent == Path(common) / 'worktrees' else []
    return res + ['--overlay-src', str(wt), '--overlay', str(tmp / 'upper'), str(tmp / 'work'), str(wt), '--chdir', str(wt),
                  '--unsetenv', 'XDG_RUNTIME_DIR', '--unsetenv', 'DBUS_SESSION_BUS_ADDRESS', '--unshare-net', '--unshare-ipc',
                  '--unshare-pid', '--unshare-uts', '--die-with-parent', '--new-session', '--', *cmd]


def remove(path):
    """Delete a scratch tree whatever modes a sandboxed run (or overlayfs, for its work directory) left on it."""
    for d, dirs, _ in os.walk(path):
        for p in (os.path.join(d, x) for x in dirs):
            if not os.path.islink(p):
                os.chmod(p, 0o700)
    shutil.rmtree(path, ignore_errors=True)


def sandboxed(cmd, wt, mb, log=None, env=None):
    """Exit status of cmd run by `sandbox` in a systemd scope capped at mb MB (and 4096 tasks), output to log or the
    terminal; 124 after TIMEOUT. The overlay's writes are discarded."""
    if not shutil.which('bwrap') or not cap.available():
        raise SystemExit('the gates need bwrap and memory-capped systemd user scopes (/setup-check)')
    tmp = Path(tempfile.mkdtemp(prefix='merge-pr-', dir='/var/tmp'))
    try:
        for d in ('tmp', 'upper', 'work'):
            (tmp / d).mkdir()
        outer = cap.capped(sandbox(cmd, wt, tmp), mb)
        outer[outer.index('--'):outer.index('--')] = ['-p', 'TasksMax=4096']
        e = {k: os.environ[k] for k in ENV if k in os.environ} | {'TMPDIR': '/tmp', 'CCACHE_DISABLE': '1', 'CP_MEMORY_CAPPED': str(mb), **(env or {})}
        with open(log, 'w') if log else contextlib.nullcontext() as f:
            try:
                return subprocess.run(outer, env=e, stdout=f, stderr=f and subprocess.STDOUT, timeout=TIMEOUT).returncode
            except subprocess.TimeoutExpired:
                print(f'\nTIMEOUT after {TIMEOUT} s', file=f or sys.stderr)
                return 124
    finally:
        remove(tmp)


def doctor_lines(text):
    """The problems `plan.py doctor` lists in its output, each with its package heading, since a line may not name one."""
    res, heading = set(), ''
    for line in text.splitlines():
        if line.startswith('## '):
            heading = line
        elif line.startswith('- '):
            res.add((heading, line))
    return res


def mb_of(wt, script):
    """The cap an entry point declares with ensure(mb=...), at most MAX_MB, else the default."""
    m = re.search(r'ensure\([^)]*mb=(\d+)', (wt / script).read_text(errors='replace'))
    return min(int(m[1]), MAX_MB) if m else cap.MAIN_MB


def suites(wt, st):
    """(suites, deleted): the testers of the package's targets in master's plan (the pull request cannot drop them by
    editing its own), of every inventory target it changes, and every tester it changes, as they exist in the worktree;
    and the testers master has for the package's targets, or the pull request changes, that are missing from it."""
    batches, packages = plan.load(ROOT)
    bmap = {b['id']: b for b in batches['batches']}
    pkg = next((p for p in packages['packages'] if p['id'] == st['package']), {'batches': []})
    own = {x for b in pkg['batches'] if b in bmap for x in bmap[b]['targets']}
    changed = git('diff', '--name-only', '--no-renames', '-z', 'master...HEAD', cwd=wt).split('\0')
    found = {t for x in own | {p for p in changed if HEADER.fullmatch(p)} for t in plan.tests_for(wt, x)} | set(changed)
    tester = lambda t: t.startswith(TESTS) and t.endswith('_tester.py')
    return (sorted(t for t in found if tester(t) and (wt / t).is_file()),
            sorted(t for t in {t for x in own for t in plan.tests_for(ROOT, x)} | set(changed) if tester(t) and not (wt / t).is_file()))


def gates(n):
    st, wt = checked_out(n)
    if git('symbolic-ref', '--short', 'HEAD') != 'master' or git('status', '--porcelain'):
        raise SystemExit('the gates compare with master: run from a clean main checkout with master checked out')
    if git('status', '--porcelain', cwd=wt):
        raise SystemExit(f'{wt} has uncommitted changes')
    if git('merge-base', '--is-ancestor', 'master', 'HEAD', cwd=wt, check=False).returncode:
        raise SystemExit(f'#{n} is not based on master: run worktree.py \'{wt}\' --sync first')
    head, master, logs = git('rev-parse', 'HEAD', cwd=wt), git('rev-parse', 'master'), STATE / str(n)
    shutil.rmtree(logs, ignore_errors=True)
    logs.mkdir(parents=True)
    py, ok = sys.executable, True

    def gate(name, cmd, mb=cap.MAIN_MB, env=None, known=frozenset()):
        nonlocal ok
        log, t = logs / (re.sub(r'[^\w.-]+', '_', name) + '.log'), time.time()
        code = sandboxed(cmd, wt, mb, log, env)
        text = log.read_text(errors='replace') if known else ''
        passed = code == 0 or code == 1 and 'Traceback' not in text and bool(doctor_lines(text)) and doctor_lines(text) <= known
        ok &= passed
        print(f'{"PASS" if passed else "FAIL"} {name} ({time.time() - t:.0f} s, exit {code}, log {log.relative_to(ROOT)})', flush=True)
        return passed

    gate('consistency', [py, TESTS + '03-consistency.py'])
    gate('plan check', [py, plan.PLAN + '/plan.py', 'check'])
    master_doctor = subprocess.run([py, str(ROOT / plan.PLAN / 'plan.py'), 'doctor'], cwd=ROOT, capture_output=True, text=True).stdout
    gate('plan doctor', [py, plan.PLAN + '/plan.py', 'doctor'], known=doctor_lines(master_doctor))
    found, deleted = suites(wt, st)
    for t in deleted:
        ok = False
        print(f'FAIL {t}: the pull request deletes a tester of its package')
    print(f'suites: {", ".join(Path(s).name for s in found)}' if found else 'suites: none (no package targets and no changed tester)')
    for s in found:
        if gate(f'{s} quick', [py, s, '--mode', 'quick', '--seed', '1'], cap.PREFLIGHT_MB):
            for cxx in ('g++', 'g++-14'):
                gate(f'{s} full {cxx}', [py, s, '--mode', 'full', '--seed', '1'], mb_of(wt, s), {'CXX': cxx})
        else:
            print(f'SKIP {s} full runs: the quick pre-flight failed')
    changed = git('diff', '--name-only', '--no-renames', '-z', 'master...HEAD', cwd=wt).split('\0')
    headers = sorted(p for p in changed if MUTATED.fullmatch(p) and (wt / p).is_file())
    subjects = lambda h: [l[1:-1] for l in git('log', '--format=<%s>', 'master..HEAD', '--', h, cwd=wt).splitlines()]  # <>: empty ones too
    packages = plan.load(ROOT)[1]['packages']
    own = {d for p in packages if p['id'] == st['package'] for d in p['evidence']}
    others = {d for p in packages if p['id'] != st['package'] for d in p['evidence']} - own
    # 05-testing.md, Mutation testing: a header only /prune or /restyle commits changed, and another package's header, keep existing records only
    loose = [h for h in headers if all(MECHANICAL.fullmatch(s) for s in subjects(h) or [''])
             or f'{h.split("/")[0]}/Docs/{Path(h).stem}.md' in others]
    for name, flag, group in (('mutation records', [], [h for h in headers if h not in loose]),
                              ('mutation records (existing only)', ['--existing'], loose)):
        if group:
            gate(name, [py, TESTS + '04-mutation.py', 'check', *flag, *group])
    if not headers:
        print('mutation records: none (no changed header)')
    script = TESTS + '02-integration.py'
    gate('integration sanitizers', [py, script, '--sanitizers'], mb_of(wt, script))
    gate('workspace check', [py, '97-Online Testing/03-workspace.py', '--check'])
    save(n, st | dict(tested=head, master=master, passed=ok))
    print(f'{"PASS" if ok else "FAIL"}: gates on {head[:12]} over master {master[:12]}')
    return 0 if ok else 1


def land(n):
    st, wt = checked_out(n)
    if not st.get('passed') or st.get('tested') != git('rev-parse', 'HEAD', cwd=wt) or st.get('master') != git('rev-parse', 'master'):
        raise SystemExit(f'the gates have not passed on this head over the current master: `worktree.py \'{wt}\' --sync` if master moved, then gates')
    pr = view(n, st['repo'], 'state,headRefOid')
    if pr['state'] != 'OPEN' or pr['headRefOid'] != st['head']:
        raise SystemExit(f'#{n} changed since files ({pr["state"].lower()}, head {pr["headRefOid"][:12]}, gated {st["head"][:12]}): drop it and start again')
    if subprocess.run([sys.executable, str(ROOT / plan.PLAN / 'worktree.py'), str(wt), '--finish'], cwd=ROOT).returncode:
        raise SystemExit('worktree.py --finish failed; master is unchanged unless it says it fast-forwarded')
    git('update-ref', '-d', f'refs/merge-pr/{n}')
    shutil.rmtree(STATE / str(n), ignore_errors=True)
    (STATE / f'{n}.json').unlink()
    sha = git('rev-parse', 'master')
    print(f'Landed #{n} on master at {sha[:12]}. For the user to run:\n  git push origin master\n'
          f'  gh pr close {n} --repo {st["repo"]} --comment "Landed on master as {sha[:12]} (rebased onto master)."')
    return 0


def drop(n):
    """Remove what files and checkout made; the branch only when checkout created it. Keeps the record on a failure."""
    st, failed = state(n), []
    steps = [(Path(st['worktree']).exists(), ('worktree', 'remove', '--force', st['worktree'])),
             (st.get('created') and git('rev-parse', '--verify', '-q', f'refs/heads/{st["branch"]}', check=False).returncode == 0,
              ('branch', '-D', st['branch'])), (True, ('update-ref', '-d', f'refs/merge-pr/{n}'))]
    for wanted, args in steps:
        r = git(*args, check=False) if wanted else None
        if r and r.returncode:
            failed.append(f'git {" ".join(args)}: ' + r.stderr.strip().replace('\n', '\n    '))
    if failed:
        raise SystemExit(f'drop #{n} incomplete; the record stays:\n  ' + '\n  '.join(failed))
    shutil.rmtree(STATE / str(n), ignore_errors=True)
    (STATE / f'{n}.json').unlink()
    print(f'dropped #{n}: worktree, ' + (f'branch {st["branch"]} and ' if st.get('created') else '') + 'fetched head removed')
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    for name in ('files', 'checkout', 'gates', 'land', 'drop'):
        sub.add_parser(name).add_argument('n', type=int)
    r = sub.add_parser('run')
    r.add_argument('n', type=int)
    r.add_argument('--mb', type=int, default=cap.MAIN_MB)
    argv = sys.argv[1:]
    cut = argv.index('--') if '--' in argv else len(argv)  # the command after `--` is never parsed as options
    a, a.command = ap.parse_args(argv[:cut]), argv[cut + 1:]
    if git('rev-parse', '--path-format=absolute', '--git-dir') != git('rev-parse', '--path-format=absolute', '--git-common-dir'):
        raise SystemExit('run /merge-pr from the main checkout, not a worktree')
    if a.cmd == 'run':
        cmd = a.command[1:] if a.command[:1] == ['--'] else a.command
        if not cmd:
            raise SystemExit('usage: merge_pr.py run N [--mb MB] -- CMD...')
        return sandboxed(cmd, checked_out(a.n)[1], min(a.mb, MAX_MB))
    return {'files': files, 'checkout': checkout, 'gates': gates, 'land': land, 'drop': drop}[a.cmd](a.n)


if __name__ == '__main__':
    sys.exit(main())
