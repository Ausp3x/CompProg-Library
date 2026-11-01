#!/usr/bin/env python3
"""Check, sync and retire package worktrees.

Usage: worktree.py [PATH] [--sync | --finish] [--integration] [--remove]
  PATH defaults to the current directory.
  (no flag)      report: clean? branch tip in master? package status on master?
                 A clean, merged worktree is DONE; an unverified package is
                 only reported, since its work is already in master.
  --sync         rebase the branch onto master. Only two conflicts are
                 resolved: packages.json, merged entry by entry and field by
                 field (a field both sides changed differently aborts) and then
                 re-ranked, and the generated plan block of 01-prompts.md,
                 re-rendered; a conflict marker left anywhere else aborts
                 the rebase and is reported. The
                 consistency validator runs before and after; new errors fail.
                 Notes other worktrees whose uncommitted edits touch the
                 files this branch changes.
  --integration  also run the integration build after a sync.
  --remove       remove the worktree and delete its branch once it is DONE.
  --finish       --sync, then fast-forward master (the main checkout must have
                 master checked out), then --remove.
  --sync, --remove and --finish must run from outside the worktree (the main
  checkout); the report works from anywhere. /finish runs --finish after
  leaving the worktree, so its own session's lock is not reported as live.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

PLAN_JSON = '00-Guidelines/13-Plan/packages.json'
GENERATED = {'01-prompts.md', PLAN_JSON}
MARKER = re.compile(r'^(<{7} |={7}$|>{7} )', re.M)
MISSING = object()


def git(*args, cwd, check=True):
    result = subprocess.run(['git', *args], cwd=cwd, capture_output=True, text=True, env=dict(os.environ, GIT_EDITOR='true'))
    if check and result.returncode:
        raise SystemExit(f'git {" ".join(args)} failed in {cwd}:\n{result.stderr.strip()}')
    return result


def validator_errors(path):
    out = subprocess.run([sys.executable, str(path / '96-Local Testing/03-consistency.py')], cwd=path, capture_output=True, text=True).stdout
    try:
        return set(json.loads(out[out.index('{'):])['errors'])
    except (ValueError, KeyError):
        return {'validator did not produce a report'}


def canon(x):
    """x as canonical JSON (so 1, 1.0 and true differ), MISSING unchanged."""
    return x if x is MISSING else json.dumps(x, sort_keys=True)


def merge3(base, ours, theirs, where, clashes):
    """Three-way merge of JSON values (MISSING: absent), compared as canonical JSON: a change on one side wins over the
    unchanged other; objects merge per key and lists of objects with a string `id` per entry; any other value both
    sides changed differently, or such a list repeating an id, is a clash, appended to `clashes` as its path."""
    b, o, t = canon(base), canon(ours), canon(theirs)
    if o == t or t == b:
        return ours
    if o == b:
        return theirs
    if all(isinstance(x, dict) for x in (base, ours, theirs)):
        merged = {k: merge3(base.get(k, MISSING), ours.get(k, MISSING), theirs.get(k, MISSING), f'{where}.{k}', clashes)
                  for k in [*ours, *(k for k in theirs if k not in ours)]}
        return {k: v for k, v in merged.items() if v is not MISSING}
    if all(isinstance(x, list) and all(isinstance(e, dict) and isinstance(e.get('id'), str) for e in x) for x in (base, ours, theirs)):
        maps = [{e['id']: e for e in x} for x in (base, ours, theirs)]
        if any(len(m) != len(x) for m, x in zip(maps, (base, ours, theirs))):
            clashes.append(f'{where} (repeated id)')
            return ours
        merged = [merge3(*(m.get(i, MISSING) for m in maps), f'{where}[{i}]', clashes) for i in order(*(list(m) for m in maps))]
        return [v for v in merged if v is not MISSING]
    clashes.append(where)
    return ours


def order(base, ours, theirs):
    """Merged id order: ours, unless only theirs reordered the ids it shares with base; an id only the other side has
    follows its nearest predecessor there that is placed."""
    def moved(x):
        sb, sx = set(base), set(x)
        return [i for i in x if i in sb] != [i for i in base if i in sx]
    first, other = (theirs, ours) if moved(theirs) and not moved(ours) else (ours, theirs)
    res = list(first)
    for k, i in enumerate(other):
        if i not in res:
            prev = next((j for j in reversed(other[:k]) if j in res), None)
            res.insert(res.index(prev) + 1 if prev else 0, i)
    return res


def merge_plan(path):
    """Resolve a conflicted packages.json from its three index stages with merge3; returns the clashes, writing the
    merged file only when there are none."""
    shown = [git('show', f':{n}:{PLAN_JSON}', cwd=path, check=False) for n in (1, 2, 3)]
    if any(s.returncode for s in shown):
        return [f'{PLAN_JSON} (added or deleted on one side)']
    try:
        base, ours, theirs = (json.loads(s.stdout) for s in shown)
    except ValueError:
        return [f'{PLAN_JSON} (a side is not JSON)']
    clashes = []
    merged = merge3(base, ours, theirs, 'packages.json', clashes)
    if not clashes:
        (path / PLAN_JSON).write_text(json.dumps(merged, indent=2, ensure_ascii=False) + '\n')
    return clashes


def inspect(path):
    facts = {'branch': git('branch', '--show-current', cwd=path).stdout.strip()}
    facts['dirty'] = git('status', '--porcelain', cwd=path).stdout.strip().splitlines()
    facts['ignored'] = [l for l in git('status', '--porcelain', '--ignored', cwd=path).stdout.splitlines() if l.startswith('!!')]
    facts['ahead'] = git('log', '--oneline', 'master..HEAD', cwd=path).stdout.strip().splitlines()
    facts['behind'] = git('log', '--oneline', 'HEAD..master', cwd=path).stdout.strip().splitlines()
    facts['merged'] = git('merge-base', '--is-ancestor', 'HEAD', 'master', cwd=path, check=False).returncode == 0
    match = re.search(r'P\d{3}', facts['branch']) or re.search(r'P\d{3}', path.name)
    facts['package'] = match.group(0) if match else None
    facts['status'] = None
    facts['note'] = ''
    if facts['package']:
        show = git('show', 'master:00-Guidelines/13-Plan/packages.json', cwd=path, check=False)
        entries = [p for p in json.loads(show.stdout)['packages'] if p['id'] == facts['package']] if show.returncode == 0 else []
        facts['status'] = entries[0]['status'] if entries else '(not in plan)'
        facts['note'] = entries[0]['note'] if entries else ''
    lock = git('worktree', 'list', '--porcelain', cwd=path).stdout
    pid = re.search(re.escape(str(path)) + r'\n[^\n]*\n[^\n]*\nlocked[^\n]*pid (\d+)', lock)
    facts['live'] = bool(pid) and Path('/proc', pid.group(1)).exists() and int(pid.group(1)) not in ancestors()
    return facts


def uncommitted(path):
    """(status, path) per uncommitted file, NUL-safe, untracked directories expanded."""
    entries, res = iter(git('--no-optional-locks', 'status', '--porcelain', '-z', '--untracked-files=all', cwd=path, check=False).stdout.split('\0')), []
    for e in entries:
        if e:
            res.append((e[:2], e[3:]))
            if 'R' in e[:2] or 'C' in e[:2]:
                res.append((e[:2], next(entries, '')))  # the rename source follows; it changes too
    return res


def overlaps(path, main_root):
    """Other worktrees whose uncommitted edits touch files this branch changes. Reading another worktree is a
    git operation a worktree session may not run, so this runs only with --sync or --finish, from outside."""
    mine = set(git('diff', '--name-only', '-z', 'master...HEAD', cwd=path).stdout.split('\0')) - {''}
    res = []
    for line in git('worktree', 'list', '--porcelain', cwd=main_root).stdout.splitlines():
        other = Path(line.split(' ', 1)[1]) if line.startswith('worktree ') else None
        if other and other not in (path, main_root) and other.is_dir():
            shared = sorted(mine & {p for _, p in uncommitted(other)})
            if shared:
                res.append(f'{other.name} has uncommitted edits to {", ".join(shared[:8])}{" ..." if len(shared) > 8 else ""}; its next rebase may conflict')
    return res


def ancestors():
    """Process ids of this process and its parents, so the calling session's own lock is not reported as live."""
    pid, seen = os.getpid(), set()
    while pid > 1 and pid not in seen:
        seen.add(pid)
        try:
            pid = int(Path(f'/proc/{pid}/stat').read_text().rsplit(')', 1)[1].split()[1])
        except (OSError, ValueError, IndexError):
            break
    return seen


def report(path, facts):
    print(f'worktree {path}\nbranch   {facts["branch"] or "(detached)"}\npackage  {facts["package"] or "(none found)"}')
    problems = []
    if not facts['branch']:
        problems.append('detached HEAD: check out or create the package branch first')
    if facts['dirty']:
        problems.append(f'uncommitted changes ({len(facts["dirty"])} paths): commit or discard them')
    if facts['merged']:
        print('merge    branch tip is contained in master')
    else:
        print(f'merge    NOT in master: {len(facts["ahead"])} commit(s) ahead, {len(facts["behind"])} behind')
        for line in facts['ahead'][:10]:
            print('           ' + line)
        if facts['behind']:
            problems.append('behind master: run `--sync` (rebase; generated plan file resolved automatically) or `--finish`')
        else:
            problems.append(f'master can fast-forward: `git merge --ff-only {facts["branch"]}` in the main checkout, or `--finish`')
    if facts['package']:
        print(f'plan     {facts["package"]} is {facts["status"]} on master'
              + ('' if facts['status'] == 'verified' else f' (removal still allowed once merged; plan note: {facts["note"] or "EMPTY, so nothing records the remaining work"})'))
    if facts['ignored']:
        print(f'note     {len(facts["ignored"])} git-ignored files present (fine; removal may need --force)')
    if facts['live']:
        print('note     the lock names a Claude session that is still running; let it finish before syncing or removing')
    return problems


def sync(path, facts, integration):
    if facts['dirty']:
        raise SystemExit('sync needs a clean worktree')
    if not facts['behind']:
        print('sync     already based on master; nothing to rebase')
    else:
        before = validator_errors(path)
        print(f'sync     rebasing {facts["branch"]} onto master ({len(facts["behind"])} new commits on master)')
        result = git('rebase', 'master', cwd=path, check=False)
        try:
            while result.returncode:
                conflicts = git('diff', '--name-only', '--diff-filter=U', cwd=path).stdout.splitlines()
                resolved = bool(conflicts) and set(conflicts) <= GENERATED
                merged = resolved and PLAN_JSON in conflicts
                if merged:
                    clashes = merge_plan(path)
                    resolved, conflicts = not clashes, clashes or conflicts
                if resolved:  # rank re-orders the merged plan as `plan.py set` would, and both commands re-render the plan block
                    subprocess.run([sys.executable, str(path / '00-Guidelines/13-Plan/plan.py'), 'rank' if merged else 'render'], cwd=path, check=True, capture_output=True)
                    conflicts = [f for f in conflicts if MARKER.search((path / f).read_text())]
                    resolved = not conflicts
                if not resolved:
                    raise SystemExit('rebase stopped on conflicts outside the generated plan block and packages.json entries; aborted. Resolve by hand in the worktree:\n  '
                                     + '\n  '.join(conflicts or [result.stderr.strip()]))
                git('add', *GENERATED, cwd=path)
                result = git('rebase', '--continue', cwd=path, check=False)
        except BaseException:  # any failure (a clash, a failing rank or render, a git error) leaves no rebase in progress
            git('rebase', '--abort', cwd=path, check=False)
            raise
        after = validator_errors(path)
        new = after - before
        if new:
            raise SystemExit('rebase done, but the validator reports new errors:\n  ' + '\n  '.join(sorted(new)))
        print('sync     rebase done; validator has no new errors' + (f' ({len(after)} pre-existing)' if after else ''))
    if integration:
        print('sync     running the integration build')
        run = subprocess.run([sys.executable, str(path / '96-Local Testing/02-integration.py')], cwd=path, capture_output=True, text=True)
        if run.returncode:
            raise SystemExit('integration build failed:\n' + (run.stdout + run.stderr)[-2000:])
        print('sync     integration build passed')


def remove(path, main_root, facts):
    git('worktree', 'unlock', str(path), cwd=main_root, check=False)
    removal = git('worktree', 'remove', str(path), cwd=main_root, check=False)
    if removal.returncode and facts['ignored']:
        print('plain removal refused (ignored files); retrying with --force')
        removal = git('worktree', 'remove', '--force', str(path), cwd=main_root, check=False)
    if removal.returncode:
        raise SystemExit('git worktree remove failed:\n' + removal.stderr.strip())
    git('branch', '-d', facts['branch'], cwd=main_root)
    print(f'removed {path} and branch {facts["branch"]}')


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('path', nargs='?', default='.')
    parser.add_argument('--sync', action='store_true')
    parser.add_argument('--finish', action='store_true')
    parser.add_argument('--integration', action='store_true')
    parser.add_argument('--remove', action='store_true')
    args = parser.parse_args()
    path = Path(git('rev-parse', '--show-toplevel', cwd=args.path).stdout.strip())
    main_root = Path(git('worktree', 'list', '--porcelain', cwd=path).stdout.split('\n', 1)[0].split(' ', 1)[1])
    if path == main_root:
        raise SystemExit('this is the main checkout, not a package worktree')
    inside = Path(os.getcwd()).resolve().is_relative_to(path)
    if (args.sync or args.finish or args.remove) and inside:
        raise SystemExit('--sync, --finish and --remove must run from outside the worktree, for example the main checkout')
    facts = inspect(path)
    if args.sync or args.finish:
        sync(path, facts, args.integration)
        facts = inspect(path)
        for line in overlaps(path, main_root):
            print('note     ' + line)
    if args.finish and not facts['merged']:
        if facts['dirty'] or facts['behind']:
            raise SystemExit('cannot fast-forward master: worktree dirty or still behind master')
        current = git('branch', '--show-current', cwd=main_root).stdout.strip()
        if current != 'master':
            raise SystemExit(f'the main checkout has {current or "a detached HEAD"} checked out, not master; switch it to master and rerun')
        git('merge', '--ff-only', facts['branch'], cwd=main_root)
        print(f'master   fast-forwarded to {facts["branch"]}')
        facts = inspect(path)
    problems = report(path, facts)
    if problems:
        print('\nNOT DONE')
        for p in problems:
            print(' - ' + p)
        return 1
    print('\nDONE: this worktree can be removed')
    if args.remove or args.finish:
        remove(path, main_root, facts)
    else:
        print(f'remove with `--remove` (or `--finish`) from the main checkout')
    return 0


if __name__ == '__main__':
    sys.exit(main())
