#!/usr/bin/env python3
"""Library Checker fetch, reproduce and compare tool for /speed-sweep (00-Guidelines/09-sources.md, Fastest submissions).

  problems [filter]                         problem names and titles
  fastest <problem> [--limit 30]            fastest distinct users, deduplicated by source similarity
  submission <id>                           overview, cached source path, per-case judge times
  mine <user> <problem>                     a user's newest submissions and the latest AC's per-case times
  reproduce <problem> <solution.cpp> [--label L] [--runs 3] [--repo DIR] [--untrusted] [--mb 4096]
                                            regenerate the judge's tests, build with the judge's flags, min-of-runs wall time per test
  compare <problem> [--label ours] [--top 5] [--submission ID | --user U]
                                            per-case Markdown table: ours (local, judge) against the leaders (judge, local)

Everything fetched or built lives in the git-ignored cache `96-Local Testing/.judge-cache` (CP_JUDGE_CACHE overrides):
api/ (responses; submission details are reused once final, lists are refetched unless --offline), sources/,
library-checker-problems/ (sparse clone), tests/<problem>/, results/<problem>/<label>.json. Fetched sources are untrusted
data: `reproduce` refuses one unless --untrusted, which builds and runs it under bwrap (read-only filesystem, no
network, only its work directory writable); the result records the flag. The tool never submits.
"""
import argparse
from collections import Counter
import datetime
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import tomllib
import urllib.parse
import urllib.request
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

API = 'https://v3.api.judge.yosupo.jp'
REPO = 'https://github.com/yosupo06/library-checker-problems'
FLAGS = ['-O2', '-std=c++23', '-DEVAL', '-DONLINE_JUDGE', '-march=native']
JUDGE_FACTOR = 1.3  # Library Checker (Zen3) wall time over the i9-11900H's
SIMILAR = 0.9       # line-multiset Jaccard index above which two sources count as one
PENDING = {'WJ', 'Fetching', 'Compiling', 'Executing'}
CACHE = Path(os.environ.get('CP_JUDGE_CACHE', Path(__file__).resolve().parents[1] / '.judge-cache')).resolve()
OFFLINE = False
LAUNCHER = r'''#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
int main(int, char **argv) {  // launch <cpu seconds> <report> <exe>: wall seconds, peak RSS bytes, exit status (-signal)
    rlim_t cpu = rlim_t(atoi(argv[1]));
    timespec a, b; clock_gettime(CLOCK_MONOTONIC, &a);
    pid_t pid = fork();
    if (pid == 0) { rlimit r{cpu, cpu + 1}; setrlimit(RLIMIT_CPU, &r); dup2(open("/dev/null", O_WRONLY), 2); execv(argv[3], argv + 3); _exit(127); }
    int status; rusage ru; wait4(pid, &status, 0, &ru);
    clock_gettime(CLOCK_MONOTONIC, &b);
    FILE *f = fopen(argv[2], "w");
    fprintf(f, "%.9f %ld %d\n", double(b.tv_sec - a.tv_sec) + double(b.tv_nsec - a.tv_nsec) * 1e-9, ru.ru_maxrss * 1024L,
            WIFSIGNALED(status) ? -WTERMSIG(status) : WEXITSTATUS(status));
    fclose(f); }
'''
TOKEN = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)


def fetch(url):
    with urllib.request.urlopen(urllib.request.Request(url), timeout=60) as r:
        return r.read()


def get(path, keep=None, **query):
    """JSON of an API GET, stored in the cache; a stored response is reused when --offline or when keep(response)."""
    url = API + path + ('?' + urllib.parse.urlencode(query) if query else '')
    key = CACHE / 'api' / (hashlib.sha1(url.encode()).hexdigest()[:20] + '.json')
    if key.exists():
        data = json.loads(key.read_text())
        if OFFLINE or (keep and keep(data)):
            return data
    if OFFLINE:
        raise SystemExit(f'--offline: {url} is not cached')
    data = json.loads(fetch(url))
    key.parent.mkdir(parents=True, exist_ok=True)
    key.write_text(json.dumps(data))
    return data


def submission(sid):
    """(details, cached source path) of a submission."""
    d = get(f'/submissions/{sid}', keep=lambda d: d['overview']['status'] not in PENDING)
    lang = d['overview']['lang']
    ext = '.cpp' if lang.startswith('cpp') else '.py' if lang.startswith(('python', 'pypy')) else '.rs' if lang == 'rust' else '.txt'
    path = CACHE / 'sources' / f'{sid}{ext}'
    if not path.exists():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(d['source'])
    return d, path


def code_lines(src, lang):
    """Multiset of a source's non-empty lines without whitespace and, for C++, without comments."""
    if lang.startswith('cpp'):
        src = TOKEN.sub(lambda m: m[0] if m[0][0] in '"\'' else '\n', src)
    return Counter(s for s in (re.sub(r'\s+', '', l) for l in src.splitlines()) if s)


def similarity(a, b):
    return sum((a & b).values()) / max(1, sum((a | b).values()))


def user(s):
    return s.get('user_name') or '(anonymous)'


def ms(t):
    return round(t * 1000)


def table(head, rows):
    print('| ' + ' | '.join(head) + ' |\n|' + '---|' * len(head))
    for r in rows:
        print('| ' + ' | '.join(map(str, r)) + ' |')


def fastest(problem, limit=30, show=True):
    """Fastest AC submissions of distinct users, merged when their sources are similar; [(submission, copies)]."""
    kept = []
    for s in get('/submissions', problem=problem, status='AC', order='+time', dedupUser='true', limit=limit)['submissions']:
        lines = code_lines(submission(s['id'])[0]['source'], s['lang'])
        same = next((k for k in kept if similarity(k[2], lines) >= SIMILAR), None)
        if same:
            same[1].append(s)
        else:
            kept.append((s, [], lines))
    if show:
        table(['#', 'id', 'user', 'lang', 'ms', 'MiB', 'date', 'same code'],
              [(i + 1, s['id'], user(s), s['lang'], ms(s['time']), f"{s['memory'] / 2**20:.1f}", s['submission_time'][:10],
                ', '.join(f"{c['id']} {user(c)}" for c in copies)) for i, (s, copies, _) in enumerate(kept)])
    return [(s, copies) for s, copies, _ in kept]


def show_submission(sid):
    d, path = submission(sid)
    o = d['overview']
    print(f"{o['id']} {o['problem_name']} {user(o)} {o['lang']} {o['status']} {ms(o['time'])} ms "
          f"{o['memory'] / 2**20:.1f} MiB {o['submission_time'][:10]}\nsource: {path} ({len(d['source'])} bytes)")
    table(['case', 'ms', 'MiB', 'status'], [(c['case'], ms(c['time']), f"{c['memory'] / 2**20:.1f}", c['status'])
                                            for c in d['case_results']])


def latest_ac(name, problem, show=False):
    """Id of a user's newest AC submission to a problem, or None."""
    subs = get('/submissions', problem=problem, user=name, order='-id', limit=20)['submissions']
    if show:
        table(['id', 'status', 'ms', 'MiB', 'date'], [(s['id'], s['status'], ms(s['time']), f"{s['memory'] / 2**20:.1f}",
                                                       s['submission_time'][:10]) for s in subs])
    return next((s['id'] for s in subs if s['status'] == 'AC'), None)


def run(cmd, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, **kw)
    if r.returncode:
        raise SystemExit(f'failed ({r.returncode}): {" ".join(map(str, cmd))}\n{r.stderr[-2000:]}')
    return r.stdout


def problem_dir(problem, repo):
    """(checkout root, problem directory); without --repo the cache's sparse clone gains the problem on first use."""
    root = Path(repo).resolve() if repo else CACHE / 'library-checker-problems'
    hits = list(root.glob(f'*/{problem}/info.toml'))
    if not hits and not repo:
        if not (root / '.git').exists():
            run(['git', 'clone', '--depth', '1', '--filter=blob:none', '--sparse', REPO, str(root)])
            run(['git', '-C', str(root), 'sparse-checkout', 'set', 'common'])
        rel = get(f'/problems/{problem}')['source_url'].split('/tree/master/', 1)[1]
        run(['git', '-C', str(root), 'sparse-checkout', 'add', rel])
        hits = list(root.glob(f'*/{problem}/info.toml'))
    if len(hits) != 1:
        raise SystemExit(f'{problem}: {len(hits)} problem directories under {root}')
    return root, hits[0].parent


def params_h(params):
    """params.h as upstream generate.py writes it."""
    fmt = lambda v: f'(long long){v}' if isinstance(v, int) else str(v) if isinstance(v, float) else f'"{v}"'
    return ''.join(f'#define {k} {fmt(v)}\n' for k, v in params.items())


def prepare(root, pdir):
    """Regenerate the judge's tests and reference answers once per info.toml; ([(case, in, ans)], time limit, checker)."""
    text = (pdir / 'info.toml').read_text()
    info, tdir = tomllib.loads(text), CACHE / 'tests' / pdir.name
    if (pdir / 'grader').exists() or any(s.get('function') for s in info.get('solutions', [])):
        raise SystemExit(f'{pdir.name}: function-style (grader) problems are not supported')
    cases = [(f'{Path(t["name"]).stem}_{i:02d}', t['name'], i) for t in info['tests'] for i in range(t['number'])]
    files = [(c, tdir / f'{c}.in', tdir / f'{c}.ans') for c, _, _ in cases]
    stamp = hashlib.sha1(text.encode()).hexdigest()
    if not (tdir / 'stamp').exists() or (tdir / 'stamp').read_text() != stamp:
        tdir.parent.mkdir(parents=True, exist_ok=True)
        tmp = Path(tempfile.mkdtemp(dir=tdir.parent))
        (pdir / 'params.h').write_text(params_h(info.get('params', {})))
        build = lambda src, exe: run(['g++', '-O2', '-std=c++17', '-I', str(root / 'common'), '-o', str(tmp / exe), str(src)])
        for name in {t['name'] for t in info['tests'] if t['name'].endswith('.cpp')}:
            build(pdir / 'gen' / name, Path(name).stem)
        build(pdir / 'sol' / 'correct.cpp', 'correct')
        build(pdir / info.get('checker', 'checker.cpp'), 'checker')
        for c, name, i in cases:
            with open(tmp / f'{c}.in', 'wb') as f:
                if name.endswith('.in'):
                    f.write((pdir / 'gen' / f'{c}.in').read_bytes())
                else:
                    gen = [sys.executable, str(pdir / 'gen' / name)] if name.endswith('.py') else [str(tmp / Path(name).stem), str(i)]
                    subprocess.run(gen, stdout=f, check=True)
            with open(tmp / f'{c}.in', 'rb') as fi, open(tmp / f'{c}.ans', 'wb') as fo:
                subprocess.run([str(tmp / 'correct')], stdin=fi, stdout=fo, check=True)
        (tmp / 'stamp').write_text(stamp)
        shutil.rmtree(tdir, ignore_errors=True)
        tmp.rename(tdir)
    return files, float(info.get('timelimit', 10)), tdir / 'checker'


def sandbox(cmd, work, tests):
    """cmd under bwrap: everything read-only, a private /tmp, no network; only `work` is writable."""
    ro = [x for p in (Path(__file__).resolve().parents[1], tests) for x in ('--ro-bind', str(p), str(p))]
    return ['bwrap', '--ro-bind', '/', '/', '--dev', '/dev', '--proc', '/proc', '--tmpfs', '/tmp', *ro, '--bind', str(work), str(work),
            '--unshare-all', '--die-with-parent', '--new-session', '--', *map(str, cmd)]


def time_cases(m):
    """{case: (minimum wall seconds over m['runs'] runs, peak RSS bytes, first nonzero exit status)}; the last output stays.

    A static launcher forks and execs the solution, since a child forked from Python inherits its RSS peak."""
    launch, report = Path(m['exe']).with_name('launch'), Path(m['exe']).with_name('report')
    if not launch.exists():
        run(['g++', '-O2', '-static', '-x', 'c++', '-o', launch, '-'], input=LAUNCHER)
    res = {}
    for name, inp, out in m['cases']:
        best, rss, code = math.inf, 0, 0
        for _ in range(m['runs']):
            with open(inp, 'rb') as fi, open(out, 'wb') as fo:
                subprocess.run([launch, str(math.ceil(2 * m['limit']) + 1), report, m['exe']], stdin=fi, stdout=fo, check=True)
            t, peak, status = report.read_text().split()
            best, rss, code = min(best, float(t)), max(rss, int(peak)), code or int(status)
        res[name] = (best, rss, code)
    return res


def reproduce(problem, solution, label=None, runs=3, untrusted=False, repo=None):
    """Time a solution on the regenerated tests; saves and prints the result, returns the number of non-AC cases."""
    solution = Path(solution).resolve()
    if CACHE.resolve() / 'sources' in solution.parents and not untrusted:
        raise SystemExit(f'{solution} is a fetched source (data only); --untrusted builds and runs it sandboxed')
    if untrusted and not shutil.which('bwrap'):
        raise SystemExit('--untrusted needs bwrap (bubblewrap)')
    root, pdir = problem_dir(problem, repo)
    files, tl, checker = prepare(root, pdir)
    (CACHE / 'run').mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(dir=CACHE / 'run'))
    try:
        exe = work / 'solution'
        wrap = (lambda cmd: sandbox(cmd, work, checker.parent)) if untrusted else list
        src = shutil.copy(solution, work) if untrusted else solution
        r = subprocess.run(wrap([os.environ.get('CXX', 'g++'), *FLAGS, '-o', exe, src]), capture_output=True, text=True)
        if r.returncode:
            raise SystemExit(f'{solution.name} does not build:\n{r.stderr[-2000:]}')
        m = dict(exe=str(exe), cases=[(c, str(i), str(work / f'{c}.out')) for c, i, _ in files], runs=runs, limit=tl)
        if untrusted:
            (work / 'manifest.json').write_text(json.dumps(m))
            run(wrap([sys.executable, '-I', Path(__file__).resolve(), '_time', work / 'manifest.json']))
            times = json.loads((work / 'times.json').read_text())
        else:
            times = time_cases(m)
        rows = []
        for c, inp, ans in files:
            t, rss, code = times[c]
            checked = lambda: subprocess.run([checker, inp, work / f'{c}.out', ans], capture_output=True).returncode == 0
            status = ('TLE' if code in (-9, -24) else 'RE') if code else 'TLE' if t > tl else 'AC' if checked() else 'WA'
            rows.append(dict(case=c, time=t, memory=rss, status=status))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    commit = subprocess.run(['git', '-C', str(root), 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip() or None
    cpu = next((l.split(':', 1)[1].strip() for l in open('/proc/cpuinfo') if l.startswith('model name')), 'unknown')
    label = re.sub(r'[^\w.-]', '_', label or solution.stem)
    result = dict(problem=problem, label=label, source=str(solution), untrusted=untrusted, runs=runs, flags=FLAGS, cpu=cpu,
                  cxx=run([os.environ.get('CXX', 'g++'), '--version']).splitlines()[0], commit=commit,
                  date=datetime.datetime.now().isoformat(timespec='seconds'), cases=rows)
    path = CACHE / 'results' / problem / f'{label}.json'
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(result, indent=1))
    table(['case', 'ms', 'MiB', 'status'], [(r['case'], f"{r['time'] * 1000:.1f}", f"{r['memory'] / 2**20:.1f}", r['status']) for r in rows])
    worst = max(r['time'] for r in rows)
    print(f'{label}: max {worst * 1000:.1f} ms (judge estimate {worst * JUDGE_FACTOR * 1000:.0f} ms), '
          f'sum {sum(r["time"] for r in rows) * 1000:.1f} ms, min of {runs}, {cpu}; saved {path}')
    return sum(r['status'] != 'AC' for r in rows)


def local(problem, label):
    path = CACHE / 'results' / problem / f'{label}.json'
    return {r['case']: (r['time'], r['status']) for r in json.loads(path.read_text())['cases']} if path.exists() else None


def judged(sid):
    return {c['case']: (c['time'], c['status']) for c in submission(sid)[0]['case_results']}


def compare(problem, label='ours', top=5, sid=None, name=None):
    """Per-case table of our local and judge times against the leaders' judge (and sandboxed local) times, in ms."""
    sid = sid or (name and latest_ac(name, problem))
    ours, judge = local(problem, label) or {}, judged(sid) if sid else {}
    cols = [('ours local', ours)] * bool(ours) + [(f'ours judge #{sid}', judge)] * bool(sid)
    lead, lead_local = [], []
    for s, _ in fastest(problem, show=False)[:top]:
        lead.append(judged(s['id']))
        cols.append((f"{user(s)} #{s['id']}", lead[-1]))
        if (theirs := local(problem, str(s['id']))):
            lead_local.append(theirs)
            cols.append((f'{user(s)} local', theirs))
    t = lambda col, c: col.get(c, (0,))[0]
    best = lambda group, c: min((t(g, c) for g in group if t(g, c)), default=0)
    gap = lambda col, group, c: f'{t(col, c) / best(group, c):.2f}' if t(col, c) and best(group, c) else '—'
    cell = lambda col, c: '—' if c not in col else f'{ms(col[c][0])}' + ('' if col[c][1] == 'AC' else f' {col[c][1]}')
    cases = list(dict.fromkeys(c for _, col in cols for c in col))
    rows = [[c, *(cell(col, c) for _, col in cols), gap(judge, lead, c), gap(ours, lead_local, c)] for c in cases]
    rows += [[f'**{f.__name__}**', *(ms(f(x for x, _ in col.values())) for _, col in cols), '', ''] for f in (max, sum)]
    table(['case', *(h for h, _ in cols), 'judge gap', 'local gap'], rows)
    if ours:
        print(f'ours local max × {JUDGE_FACTOR} ≈ {ms(max(t for t, _ in ours.values()) * JUDGE_FACTOR)} ms judge estimate; '
              'judge gap = ours judge / best leader judge, local gap = ours local / best leader local')


def main(argv=None):
    global OFFLINE
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--offline', action='store_true', help='use cached API responses only')
    sub = p.add_subparsers(dest='cmd', required=True)
    sub.add_parser('problems').add_argument('filter', nargs='?', default='')
    q = sub.add_parser('fastest')
    q.add_argument('problem')
    q.add_argument('--limit', type=int, default=30)
    sub.add_parser('submission').add_argument('id', type=int)
    q = sub.add_parser('mine')
    q.add_argument('user')
    q.add_argument('problem')
    q = sub.add_parser('reproduce')
    q.add_argument('problem')
    q.add_argument('solution')
    q.add_argument('--label')
    q.add_argument('--runs', type=int, default=3)
    q.add_argument('--repo', help='existing library-checker-problems checkout')
    q.add_argument('--untrusted', action='store_true', help='build and run a fetched source under bwrap')
    q.add_argument('--mb', type=int, default=4096, help='memory cap of the whole reproduction')
    q = sub.add_parser('compare')
    q.add_argument('problem')
    q.add_argument('--label', default='ours')
    q.add_argument('--top', type=int, default=5)
    g = q.add_mutually_exclusive_group()
    g.add_argument('--submission', type=int)
    g.add_argument('--user')
    sub.add_parser('_time').add_argument('manifest')
    a = p.parse_args(argv)
    OFFLINE = a.offline
    if a.cmd == '_time':
        m = Path(a.manifest)
        m.with_name('times.json').write_text(json.dumps(time_cases(json.loads(m.read_text()))))
    elif a.cmd == 'problems':
        table(['name', 'title'], [(x['name'], x['title']) for x in get('/problems')['problems'] if a.filter in x['name']])
    elif a.cmd == 'fastest':
        fastest(a.problem, a.limit)
    elif a.cmd == 'submission':
        show_submission(a.id)
    elif a.cmd == 'mine':
        if (sid := latest_ac(a.user, a.problem, show=True)):
            show_submission(sid)
    elif a.cmd == 'reproduce':
        _00_memory_cap.ensure(force=True, mb=a.mb)
        return int(reproduce(a.problem, a.solution, a.label, a.runs, a.untrusted, a.repo) > 0)
    else:
        compare(a.problem, a.label, a.top, a.submission, a.user)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
