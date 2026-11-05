#!/usr/bin/env python3
"""Checks for .claude/hooks/shell.py (the Bash pre/post hook) and the validator entry of .claude/hooks/consistency.py.

Every case runs copies of both hooks in a temporary `git init` repository with a stub `96-Local Testing/03-consistency.py`
(logs each run to HOOK_LOG; fails with the lines of HOOK_FAIL when that file exists) and a stub
`97-Online Testing/03-workspace.py` (`--check` passes only for template.cpp reading 'generated'). A "command" is
`shell.py pre`, then file operations done here, then `shell.py post`, with the same tool_use_id. The real repository is
only read (its .claude/settings.json).
quick: every fixed case. full: quick plus 40 seeded random rounds of 6 commands over random writes, deletions,
restores, new files, staging and commits, with outside changes between commands. stress: full with --rounds rounds
(default 300). Modes/seeds also come from CP_TEST_MODE/CP_TEST_SEED.
Oracle (written here, independent of shell.py): a model of HEAD and working-tree contents; a path changed when its
content (or absence) before the command differs from after; it is blocked when it then differs from its content at the
HEAD the command started on and lies under OLD/ (00-index.md excepted) or is a present 97-Legacy/ file (00-index.md
excepted), or when it is template.cpp and does not read 'generated'; the validator runs when a changed path ends in .md or .hpp, lies in 00-Guidelines/13-Plan/, or is
a .py file in 08-Python/. Failing random rounds print the seed, mode, round and the command log.

Feature map:
no change, read-only command -> test_readonly;
OLD/ edit, deletion, new file, rename, 00-index.md -> test_old;
97-Legacy/ edit, new file, deletion (unlink, git rm), 00-index.md, top-level and nested folders -> test_legacy;
template.cpp hand edit, regenerated, deleted -> test_template;
validator on .md, .hpp, plan file, 08-Python module; not on other files; failure text as additionalContext; block and
context together -> test_validator, test_block_and_context;
content hashes (a dirty file edited again; staging and commits run nothing; a protected file restored; an untouched
dirty protected file) -> test_content, test_restore_and_preexisting;
edits a command also commits (validator, block with 'amend or revert', excerpt deletion committed) -> test_committed;
restore commands (git restore from the old HEAD for tracked, rm for untracked, shell quoting) -> test_old, test_quoting;
state (no snapshot -> nothing, malformed input and unknown mode exit 0 silently, snapshot removed after post, separate
tool_use_id files, stale snapshots pruned on pre) -> test_state;
consistency.py Edit/Write entry (watched paths only, failure as additionalContext) -> test_consistency_edit;
git push in command position refused by pre without a snapshot (after separators, newlines, subshells, assignments and
env/sudo prefixes, git options and -C/-c values between, after a heredoc or here-string ends), quoted text, heredoc
bodies (quoted, <<- with tabs, two on one line), other git subcommands and unparsable commands allowed -> test_push;
the real settings wire shell.py pre/post to Bash beside guard.py and consistency.py -> test_settings;
random command sequences against the oracle -> test_random.
"""
import argparse
import json
import os
from pathlib import Path
import random
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

ROOT = Path(__file__).resolve().parents[2]
HOOKS = ROOT / '.claude/hooks'
MODE, SEED, ROUNDS = 'full', 42, 300
TEMPLATE = '99-Workspace/template.cpp'
FILES = {'OLD/00-index.md': 'old index\n', 'OLD/a.cpp': 'old a\n', '97-Legacy/00-index.md': 'top legacy\n', '97-Legacy/t.cpp': 'top\n',
         '05-M/97-Legacy/00-index.md': 'legacy index\n', '05-M/97-Legacy/x.cpp': 'legacy x\n', TEMPLATE: 'generated\n',
         '05-M/00-index.md': 'rows\n', '05-M/01-a.hpp': '#pragma once\n', '08-Python/_01_a.py': 'a = 1\n', 'tools/t.py': 't\n',
         '00-Guidelines/13-Plan/packages.json': '{}\n', 'notes.txt': 'n\n', '.gitignore': '__pycache__/\n.claude/hooks/.state/\n'}
VALIDATOR = '''import json, os, sys
open(os.environ['HOOK_LOG'], 'a').write('validate\\n')
f = os.environ['HOOK_FAIL']
bad = [l for l in open(f).read().splitlines() if l] if os.path.exists(f) else []
print(json.dumps({'errors': bad}))
sys.exit(1 if bad else 0)
'''
WORKSPACE = '''import pathlib, sys
p = pathlib.Path(__file__).resolve().parents[1] / '99-Workspace/template.cpp'
sys.exit(0 if sys.argv[1:] == ['--check'] and p.is_file() and p.read_text() == 'generated\\n' else 1)
'''


def git(root, *args):
    return subprocess.run(['git', *args], cwd=root, capture_output=True, text=True, check=True).stdout


def o_watched(p):
    return p.endswith(('.md', '.hpp')) or p.startswith('00-Guidelines/13-Plan/') or (p.startswith('08-Python/') and p.endswith('.py'))


def o_blocked(p, after, base):
    if after == base.get(p):
        return False
    if p == TEMPLATE:
        return after != 'generated\n'
    if p.startswith('OLD/'):
        return p != 'OLD/00-index.md'
    return ('/97-Legacy/' in '/' + p) and not p.endswith('97-Legacy/00-index.md') and after is not None


class Hooks(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix='hooks-test-'))
        self.repo = self.tmp / 'repo'
        for rel, text in FILES.items():
            self.write(rel, text)
        for name in ('shell.py', 'consistency.py'):
            self.write(f'.claude/hooks/{name}', (HOOKS / name).read_text())
        self.write('96-Local Testing/03-consistency.py', VALIDATOR)
        self.write('97-Online Testing/03-workspace.py', WORKSPACE)
        git(self.repo, 'init', '-q', '-b', 'master')
        git(self.repo, 'config', 'user.name', 'T')
        git(self.repo, 'config', 'user.email', 't@example.invalid')
        git(self.repo, 'add', '-A')
        git(self.repo, 'commit', '-q', '-m', 'init')
        self.log, self.fail = self.tmp / 'log', self.tmp / 'fail'
        self.env = {k: v for k, v in os.environ.items() if not k.startswith('GIT_')} | {
            'HOOK_LOG': str(self.log), 'HOOK_FAIL': str(self.fail), 'GIT_CONFIG_GLOBAL': os.devnull, 'GIT_CONFIG_NOSYSTEM': '1'}
        self.n = 0

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def write(self, rel, text):
        (self.repo / rel).parent.mkdir(parents=True, exist_ok=True)
        (self.repo / rel).write_text(text)

    def hook(self, script, *args, payload=None, raw=None):
        r = subprocess.run([sys.executable, str(self.repo / '.claude/hooks' / script), *args], input=raw if raw is not None else json.dumps(payload),
                           capture_output=True, text=True, env=self.env, timeout=120)
        self.assertEqual((r.returncode, r.stderr), (0, ''), r)
        return json.loads(r.stdout) if r.stdout.strip() else None

    def command(self, *ops, tid=None):
        """(hook output or None, whether the validator ran) of one Bash command made of ops."""
        self.n += 1
        payload = {'session_id': 's1', 'tool_use_id': tid or f'toolu_{self.n}', 'tool_name': 'Bash', 'tool_input': {'command': 'x'}}
        self.log.unlink(missing_ok=True)
        self.hook('shell.py', 'pre', payload=payload)
        for op in ops:
            op()
        out = self.hook('shell.py', 'post', payload=payload)
        return out, self.log.exists()

    def edit(self, rel, text):
        return lambda: self.write(rel, text)

    def delete(self, rel):
        return lambda: (self.repo / rel).unlink()

    def gitop(self, *args):
        return lambda: git(self.repo, *args)

    def blocked(self, out):
        return out and out.get('decision') == 'block' and out['reason']

    def test_readonly(self):
        self.assertEqual(self.command(), (None, False))
        self.assertEqual(self.command(self.gitop('status'), lambda: (self.repo / 'OLD/a.cpp').read_text()), (None, False))

    def test_old(self):
        out, ran = self.command(self.edit('OLD/a.cpp', 'changed\n'))
        reason = self.blocked(out)
        self.assertIn('This command changed protected files: OLD/a.cpp.', reason)
        self.assertIn(f'Restore: git restore --source={self.head()[:12]} --staged --worktree -- OLD/a.cpp.', reason)
        self.assertNotIn('amend', reason)
        self.assertFalse(ran)
        git(self.repo, 'checkout', '--', 'OLD/a.cpp')
        self.assertIn('OLD/a.cpp', self.blocked(self.command(self.delete('OLD/a.cpp'))[0]))
        git(self.repo, 'checkout', '--', 'OLD/a.cpp')
        reason = self.blocked(self.command(self.edit('OLD/new.cpp', 'x\n'))[0])
        self.assertIn('Restore: rm -- OLD/new.cpp', reason)
        self.assertNotIn('git restore', reason)
        (self.repo / 'OLD/new.cpp').unlink()
        reason = self.blocked(self.command(self.gitop('mv', 'OLD/a.cpp', 'OLD/b.cpp'))[0])
        self.assertIn('OLD/a.cpp, OLD/b.cpp', reason)
        git(self.repo, 'reset', '-q', '--hard')
        self.assertEqual(self.command(self.edit('OLD/00-index.md', 'more\n')), (None, True))

    def test_legacy(self):
        for rel in ('05-M/97-Legacy/x.cpp', '97-Legacy/t.cpp'):
            self.assertIn(rel, self.blocked(self.command(self.edit(rel, 'edited\n'))[0]))
            git(self.repo, 'checkout', '--', rel)
        self.assertIn('rm -- 05-M/97-Legacy/new.cpp', self.blocked(self.command(self.edit('05-M/97-Legacy/new.cpp', 'n\n'))[0]))
        (self.repo / '05-M/97-Legacy/new.cpp').unlink()
        self.assertEqual(self.command(self.delete('05-M/97-Legacy/x.cpp')), (None, False))
        self.assertEqual(self.command(self.gitop('rm', '-q', '97-Legacy/t.cpp')), (None, False))
        self.assertEqual(self.command(self.edit('05-M/97-Legacy/00-index.md', 'fewer\n')), (None, True))
        git(self.repo, 'reset', '-q', '--hard')
        self.assertIn('05-M/97-Legacy/x.cpp', self.blocked(self.command(self.delete('05-M/97-Legacy/x.cpp'),
                                                                           self.edit('05-M/97-Legacy/x.cpp', 'back but different\n'))[0]))

    def test_template(self):
        self.assertIn(TEMPLATE, self.blocked(self.command(self.edit(TEMPLATE, 'hand\n'))[0]))
        self.assertEqual(self.command(self.edit(TEMPLATE, 'generated\n')), (None, False), 'regenerated')
        self.write(TEMPLATE, 'stale\n')
        self.assertEqual(self.command(self.edit(TEMPLATE, 'generated\n')), (None, False), 'regenerated over a stale copy')
        self.assertIn(TEMPLATE, self.blocked(self.command(self.delete(TEMPLATE))[0]))

    def test_validator(self):
        for rel in ('05-M/00-index.md', '05-M/01-a.hpp', '00-Guidelines/13-Plan/packages.json', '08-Python/_01_a.py', 'docs/new.md'):
            self.assertEqual(self.command(self.edit(rel, 'v1\n')), (None, True), rel)
        for rel in ('tools/t.py', 'notes.txt', '05-M/x.cpp'):
            self.assertEqual(self.command(self.edit(rel, 'v1\n')), (None, False), rel)
        self.fail.write_text('Missing row: x\nBad link: y\n')
        out, ran = self.command(self.edit('05-M/00-index.md', 'v2\n'))
        self.assertTrue(ran)
        self.assertEqual(out, {'hookSpecificOutput': {'hookEventName': 'PostToolUse',
                                                      'additionalContext': 'Consistency validator FAILED:\nMissing row: x\nBad link: y'}})

    def test_block_and_context(self):
        self.fail.write_text('E\n')
        out, ran = self.command(self.edit('OLD/a.cpp', 'c\n'), self.edit('05-M/00-index.md', 'c\n'))
        self.assertTrue(ran and self.blocked(out))
        self.assertEqual(out['hookSpecificOutput']['additionalContext'], 'Consistency validator FAILED:\nE')

    def test_content(self):
        self.write('05-M/00-index.md', 'dirty\n')
        self.assertEqual(self.command(self.edit('05-M/00-index.md', 'dirtier\n')), (None, True), 'a dirty file edited again')
        self.assertEqual(self.command(self.edit('05-M/00-index.md', 'dirtier\n')), (None, False), 'same bytes rewritten')
        self.assertEqual(self.command(self.gitop('add', '-A')), (None, False), 'staging')
        self.assertEqual(self.command(self.gitop('commit', '-q', '-m', 'c')), (None, False), 'commit')

    def head(self):
        return git(self.repo, 'rev-parse', 'HEAD').strip()

    def test_committed(self):
        start = self.head()
        self.assertEqual(self.command(self.edit('05-M/00-index.md', 'c\n'), self.gitop('commit', '-qam', 'c')), (None, True))
        self.assertEqual(self.command(self.edit('notes.txt', 'c\n'), self.gitop('commit', '-qam', 'c')), (None, False))
        start = self.head()
        out, ran = self.command(self.edit('OLD/a.cpp', 'c\n'), self.gitop('commit', '-qam', 'c'))
        reason = self.blocked(out)
        self.assertIn(f'Restore: git restore --source={start[:12]} --staged --worktree -- OLD/a.cpp; then amend or revert the commit that holds them.', reason)
        self.assertFalse(ran)
        git(self.repo, 'restore', f'--source={start}', '--staged', '--worktree', '--', 'OLD/a.cpp')
        self.assertEqual((self.repo / 'OLD/a.cpp').read_text(), 'old a\n', 'the printed restore works')
        git(self.repo, 'commit', '-qm', 'undo')
        self.assertEqual(self.command(self.gitop('rm', '-q', '05-M/97-Legacy/x.cpp'), self.gitop('commit', '-qm', 'rm')), (None, False))
        self.assertIn('05-M/97-Legacy/new.cpp', self.blocked(self.command(self.edit('05-M/97-Legacy/new.cpp', 'n\n'), self.gitop('add', '-A'),
                                                                             self.gitop('commit', '-qm', 'add'))[0]))

    def test_restore_and_preexisting(self):
        self.write('OLD/a.cpp', 'user edit\n')
        self.assertEqual(self.command(self.edit('notes.txt', 'other\n')), (None, False), 'an untouched dirty protected file')
        self.assertEqual(self.command(self.gitop('checkout', '--', 'OLD/a.cpp')), (None, False), 'restored to HEAD')

    def test_quoting(self):
        rel = "05-M/97-Legacy/it's a.cpp"
        reason = self.blocked(self.command(self.edit(rel, 'q\n'))[0])
        self.assertIn("rm -- '05-M/97-Legacy/it'\"'\"'s a.cpp'", reason)

    def test_state(self):
        state = self.repo / '.claude/hooks/.state'
        payload = {'session_id': 's', 'tool_use_id': 'toolu_x', 'tool_name': 'Bash', 'tool_input': {}}
        self.write('OLD/a.cpp', 'no snapshot\n')
        self.assertIsNone(self.hook('shell.py', 'post', payload=payload), 'post without pre')
        self.assertIsNone(self.hook('shell.py', 'post', raw='not json'))
        self.assertIsNone(self.hook('shell.py', 'other', payload=payload))
        self.assertIsNone(self.hook('shell.py', raw=''))
        git(self.repo, 'checkout', '--', 'OLD/a.cpp')
        a = payload | {'tool_use_id': 'toolu_a'}
        b = payload | {'tool_use_id': 'toolu_b'}
        self.hook('shell.py', 'pre', payload=a)
        self.write('OLD/a.cpp', 'between\n')
        self.hook('shell.py', 'pre', payload=b)
        self.assertEqual(sorted(p.name for p in state.iterdir()), ['toolu_a.json', 'toolu_b.json'])
        self.assertIn('OLD/a.cpp', self.blocked(self.hook('shell.py', 'post', payload=a)))
        self.assertIsNone(self.hook('shell.py', 'post', payload=b), 'b started after the change')
        self.assertEqual(list(state.iterdir()), [])
        stale, fresh = state / 'old.json', state / 'young.json'
        for f in (stale, fresh):
            f.write_text('{}')
        os.utime(stale, (time.time() - 2 * 86400,) * 2)
        self.hook('shell.py', 'pre', payload=a)
        self.assertEqual(sorted(p.name for p in state.iterdir()), ['toolu_a.json', 'young.json'])

    def test_consistency_edit(self):
        for rel, ran in (('05-M/00-index.md', True), ('05-M/01-a.hpp', True), ('notes.txt', False), (str(self.tmp / 'outside.md'), False)):
            self.log.unlink(missing_ok=True)
            path = rel if rel.startswith('/') else str(self.repo / rel)
            self.assertIsNone(self.hook('consistency.py', payload={'tool_input': {'file_path': path}}))
            self.assertEqual(self.log.exists(), ran, rel)
        self.fail.write_text('F\n')
        self.assertEqual(self.hook('consistency.py', payload={'tool_input': {'file_path': '05-M/00-index.md'}}),
                         {'hookSpecificOutput': {'hookEventName': 'PostToolUse', 'additionalContext': 'Consistency validator FAILED:\nF'}})
        self.assertIsNone(self.hook('consistency.py', raw='{}'))

    def test_push(self):
        deny = {'hookSpecificOutput': {'hookEventName': 'PreToolUse', 'permissionDecision': 'deny', 'permissionDecisionReason':
                                       "Pushing is the user's (CLAUDE.md, Git): print the push command for them to run instead."}}
        state = self.repo / '.claude/hooks/.state'
        git_push = 'git' + ' push'
        for cmd in (git_push, git_push + ' origin master', 'git -C /x push', 'cd x && ' + git_push, 'git -c a=b push -u o b',
                    'ls; git --no-pager push', '/usr/bin/' + git_push, 'GIT_DIR=x ' + git_push, 'env A=1 ' + git_push, 'sudo ' + git_push,
                    'echo a\n' + git_push, f'({git_push})', f'true || {git_push} --force', f'if x; then {git_push}; fi', 'git -C "my dir" push',
                    f"cat <<'EOF'\nx\nEOF\n{git_push}", f'cat <<< x; {git_push}'):
            payload = {'session_id': 's', 'tool_use_id': 'toolu_p', 'tool_name': 'Bash', 'tool_input': {'command': cmd}}
            self.assertEqual(self.hook('shell.py', 'pre', payload=payload), deny, cmd)
            self.assertFalse((state / 'toolu_p.json').exists(), cmd)
        for cmd in ('git stash push', f'git commit -m "{git_push} docs"', 'echo ' + git_push, 'git log --grep=push', "echo 'x", 'gh pr ready 4',
                    f'printf "%s\\n" "{git_push}"', 'git config --get remote.origin.pushurl', 'ls push', 'git -C push status',
                    f"python3 - <<'EOF'\n{git_push}\nx; {git_push}\nEOF", f'cat <<-END\n\t{git_push}\n\tEND',
                    f'cat <<A <<"B"\n{git_push}\nA\n{git_push}\nB', f'cat <<< "{git_push}"'):
            payload = {'session_id': 's', 'tool_use_id': 'toolu_q', 'tool_name': 'Bash', 'tool_input': {'command': cmd}}
            self.assertIsNone(self.hook('shell.py', 'pre', payload=payload), cmd)
            self.assertTrue((state / 'toolu_q.json').exists(), cmd)
            self.assertIsNone(self.hook('shell.py', 'post', payload=payload), cmd)

    def test_settings(self):
        hooks = json.loads((ROOT / '.claude/settings.json').read_text())['hooks']
        cmds = {ev: {(h['matcher'], x['command']) for h in hooks[ev] for x in h['hooks']} for ev in ('PreToolUse', 'PostToolUse')}
        self.assertIn(('Bash', 'python3 "$CLAUDE_PROJECT_DIR/.claude/hooks/shell.py" pre'), cmds['PreToolUse'])
        self.assertIn(('Bash', 'python3 "$CLAUDE_PROJECT_DIR/.claude/hooks/shell.py" post'), cmds['PostToolUse'])
        self.assertIn(('Edit|Write|Read|NotebookEdit', 'python3 "$CLAUDE_PROJECT_DIR/.claude/hooks/guard.py"'), cmds['PreToolUse'])
        self.assertIn(('Edit|Write', 'python3 "$CLAUDE_PROJECT_DIR/.claude/hooks/consistency.py"'), cmds['PostToolUse'])

    def test_random(self):
        rounds = 0 if MODE == 'quick' else 40 if MODE == 'full' else ROUNDS
        paths = [p for p in FILES if p != '.gitignore'] + ['OLD/n.cpp', '05-M/97-Legacy/n.cpp', '05-M/02-b.md', 'tools/n.txt']
        for r in range(rounds):
            rng = random.Random(SEED * 1_000_003 + r)
            self.tearDown()
            self.setUp()
            head = {p: t for p, t in FILES.items()}
            work = dict(head)
            history = []
            for c in range(6):
                if rng.random() < 0.4:  # an outside change between commands
                    p = rng.choice(paths)
                    self.write(p, rng.choice(['outside\n', FILES.get(p, 'x\n')]))
                    work[p] = (self.repo / p).read_text()
                    history.append(('outside', p))
                ops, before, base, start = [], dict(work), dict(head), self.head()
                for _ in range(rng.randint(0, 3)):
                    p, kind = rng.choice(paths), rng.choice(['write', 'write', 'delete', 'restore', 'stage', 'commit'])
                    if kind == 'write':
                        text = rng.choice(['a\n', 'b\n', 'generated\n', FILES.get(p, 'x\n')])
                        ops.append(self.edit(p, text))
                        work[p] = text
                    elif kind == 'delete' and work.get(p) is not None:
                        ops.append(self.delete(p))
                        work[p] = None
                    elif kind == 'restore' and head.get(p) is not None:
                        ops.append(self.gitop('checkout', 'HEAD', '--', p))
                        work[p] = head[p]
                    elif kind == 'stage':
                        ops.append(self.gitop('add', '-A'))
                    elif kind == 'commit':
                        ops.append(self.gitop('add', '-A'))
                        ops.append(self.gitop('commit', '-q', '--allow-empty', '-m', 'r'))
                        head = {q: t for q, t in work.items() if t is not None}
                    else:
                        continue
                    history.append((kind, p))
                out, ran = self.command(*ops)
                changed = {p for p in set(before) | set(work) if before.get(p) != work.get(p)}
                want_bad = sorted(p for p in changed if o_blocked(p, work.get(p), base))
                info = f'seed={SEED} mode={MODE} round={r} command={c} history={history}'
                got_bad = sorted(set(out['reason'].split(': ', 1)[1].split('. OLD/', 1)[0].split(', '))) if self.blocked(out) else []
                self.assertEqual(got_bad, want_bad, info)
                self.assertEqual(ran, any(o_watched(p) for p in changed), info)
                if not want_bad and out:
                    self.fail(f'unexpected output {out}; {info}')
                if want_bad:  # put the protected files back as the reason says, outside the hooks, and commit that
                    for p in want_bad:
                        self.write(p, base[p]) if p in base else (self.repo / p).unlink(missing_ok=True)
                        work[p] = base.get(p)
                    git(self.repo, 'add', '-A')
                    git(self.repo, 'commit', '-q', '--allow-empty', '-m', 'restore')
                    head = {q: t for q, t in work.items() if t is not None}
                    history.append(('restored', want_bad, start[:8]))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')))
    parser.add_argument('--rounds', type=int, default=300, help='random rounds in stress mode')
    args = parser.parse_args()
    MODE, SEED, ROUNDS = args.mode, args.seed, args.rounds
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(Hooks)
    print(f'hook checks: mode={MODE} seed={SEED} tests={suite.countTestCases()}', flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
