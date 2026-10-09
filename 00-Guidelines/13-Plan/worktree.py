#!/usr/bin/env python3
"""Check, sync and retire package worktrees.

Usage: worktree.py [PATH] [--sync | --finish] [--integration] [--remove]
  PATH defaults to the current directory.
  (no flag)      report: clean? branch tip in master? package verified on master?
  --sync         rebase the branch onto master. Only the generated plan file
                 (01-prompts.md) is resolved automatically, by re-rendering it;
                 any other conflict aborts the rebase and is reported. The
                 consistency validator runs before and after; new errors fail.
  --integration  also run the integration build after a sync.
  --remove       remove the worktree and delete its branch once it is DONE.
  --finish       --sync, then fast-forward master, then --remove.
  --sync, --remove and --finish must run from outside the worktree (the main
  checkout); the report works from anywhere.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

GENERATED = {'01-prompts.md'}


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
    if facts['package']:
        show = git('show', 'master:00-Guidelines/13-Plan/packages.json', cwd=path, check=False)
        entries = [p for p in json.loads(show.stdout)['packages'] if p['id'] == facts['package']] if show.returncode == 0 else []
        facts['status'] = entries[0]['status'] if entries else '(not in plan)'
    lock = git('worktree', 'list', '--porcelain', cwd=path).stdout
    pid = re.search(re.escape(str(path)) + r'\n[^\n]*\n[^\n]*\nlocked[^\n]*pid (\d+)', lock)
    facts['live'] = bool(pid) and Path('/proc', pid.group(1)).exists()
    return facts


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
        print(f'plan     {facts["package"]} is {facts["status"]} on master')
        if facts['status'] != 'verified':
            problems.append(f'{facts["package"]} is {facts["status"]} on master, not verified')
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
        while result.returncode:
            conflicts = git('diff', '--name-only', '--diff-filter=U', cwd=path).stdout.split()
            if not conflicts or not set(conflicts) <= GENERATED:
                git('rebase', '--abort', cwd=path, check=False)
                raise SystemExit('rebase stopped on conflicts outside the generated plan file; aborted. Resolve by hand in the worktree:\n  '
                                 + '\n  '.join(conflicts or [result.stderr.strip()]))
            subprocess.run([sys.executable, str(path / '00-Guidelines/13-Plan/plan.py'), 'render'], cwd=path, check=True, capture_output=True)
            git('add', *conflicts, cwd=path)
            result = git('rebase', '--continue', cwd=path, check=False)
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
    if args.finish and not facts['merged']:
        if facts['dirty'] or facts['behind']:
            raise SystemExit('cannot fast-forward master: worktree dirty or still behind master')
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
