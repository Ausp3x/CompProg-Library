#!/usr/bin/env python3
"""Checks for .claude/hooks/shell.py (the Bash pre/post hook), the validator entry of .claude/hooks/consistency.py, and
(SUP32) the PreCompact mode of .claude/hooks/resume.py and the Notification hook .claude/hooks/notify.py.

Every case runs copies of both hooks in a temporary `git init` repository with a stub `96-Local Testing/03-consistency.py`
(logs each run to HOOK_LOG; fails with the lines of HOOK_FAIL when that file exists, and lists the lines of
HOOK_FAIL.reattach as skill_reattach warnings) and a stub
`97-Online Testing/03-workspace.py` (`--check` passes only for template.cpp reading 'generated'). A "command" is
`shell.py pre`, then file operations done here, then `shell.py post`, with the same tool_use_id. The real repository is
only read (its .claude/settings.json). resume.py runs as a copy in the same repository beside a stub
.claude/skills/finish/gaps.py whose target(root, arg='') has the real return shape (key, package dict, batch ids,
warning) over the fixed ids SUP32 (a batch of P900), P900 and P018, a batch id winning, arg before the branch name,
and whose ledger_rows is the real gaps.py's (loaded from the real repository);
notify.py runs as a copy with a fake notify-send first on PATH that logs its argv (NOTIFY_EXIT, NOTIFY_SLEEP), or is
imported for its macOS and Windows argv and its functions (its DEADLINE moved an hour ahead, since the imported
module's 9 s count from the import). The hooks-path cases copy the real setup_check.py beside the hooks.
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
the memory-cap note on compiles and built-binary runs, none on mentions, queries, wrapped commands and scripts -> test_memory_cap_note;
OLD/ edit, deletion, new file, rename, 00-index.md -> test_old;
97-Legacy/ edit, new file, deletion (unlink, git rm), 00-index.md, top-level and nested folders -> test_legacy;
template.cpp hand edit, regenerated, deleted -> test_template;
validator on .md, .hpp, plan file, 08-Python module; not on other files; failure text as additionalContext; block and
context together -> test_validator, test_block_and_context;
content hashes (a dirty file edited again; staging and commits run nothing; a protected file restored; an untouched
dirty protected file) -> test_content, test_restore_and_preexisting;
edits a command also commits (validator, block with 'amend or revert', excerpt deletion committed) -> test_committed;
restore commands (git restore from the old HEAD for tracked, rm for untracked, shell quoting) -> test_old, test_quoting;
validator additionalContext named by the input's hook_event_name (PostToolUseFailure; PostToolUse when missing or
empty) -> test_event_name;
core.hooksPath (pre stores [HEAD, paths, value or null]; post blocks when the command unset it, made it relative or
pointed it elsewhere, the reason naming `! git config core.hooksPath <path>`; no block for another spelling of the same
absolute path, a restore, an unchanged value even when off, or an old two-element state; the protected-files reason
first and this one after it, the validator context beside) -> test_hooks_path;
state (no snapshot -> nothing, malformed input and unknown mode exit 0 silently, snapshot removed after post, separate
tool_use_id files, stale snapshots pruned on pre) -> test_state;
consistency.py PostToolBatch entry (one run per batch with an Edit or Write call on a watched path, relative or absolute;
other tools, other paths, paths outside the root (also through ..) and malformed calls run nothing; input without a
tool_calls list, a PostToolUse payload included, gets a notice and no run; unreadable input is silent; failure as
additionalContext; skill_reattach warnings after it only for a batch that edited a .claude/skills/.../SKILL.md, alone
when the validator passes, silent without warnings) -> test_consistency_batch;
git push in command position refused by pre without a snapshot (after separators, newlines, subshells, assignments and
env/sudo prefixes, git options and -C/-c values between, after a heredoc or here-string ends), quoted text, heredoc
bodies (quoted, <<- with tabs, two on one line), other git subcommands and unparsable commands allowed -> test_push;
04-submit.py refused by pre without a snapshot when run directly or through python/pypy (versioned, by path, with options
and -X/-W values between, after separators, prefixes and assignments); reading, staging, compiling or testing it, other
scripts, quoted text and heredoc bodies allowed -> test_submit;
the pre-push hook through real pushes to a bare remote (plain, `sh -c` and variable forms): refused with
CLAUDE_CODE_CHILD_SESSION=1 unless CP_USER=1, allowed outside it, with CLAUDECODE alone or another CP_USER value refused,
the remote untouched by refusals; pre, for Bash and Monitor, refuses commands naming CP_USER, CLAUDE_CODE_CHILD_SESSION,
--no-verify or a core.hooksPath value (also quoted, heredoc, git config environment), not longer names or a read of the
key; pushes after env and sudo options; a Monitor call gets no snapshot -> test_pre_push;
the real settings wire shell.py pre/post to Bash beside guard.py, and consistency.py to PostToolBatch only -> test_settings;
random command sequences against the oracle -> test_random;
shell.py commit through the real .claude/githooks/pre-commit as git runs it (core.hooksPath absolute): staged OLD/ and
97-Legacy/ rules with the same exceptions, template.cpp regenerated, hand-edited, staged but edited again, deleted;
validator on watched staged paths only, its failure refused with its lines; a staged watched file edited again or
deleted from the index while its working copy stays is refused without running the validator; a partial commit (`git commit -- <path>`)
checks only its paths, `commit -a` all; the first commit (no HEAD); a package worktree runs the validator in its own
checkout; a /merge-pr worktree (.claude/worktrees/PR<N>) gets the path rules only and its own copy of the hook never
runs; an internal error refuses -> test_commit, test_commit_worktrees, test_commit_error; shell.py range <base> (the CI
path check) over commits made without the hook: protected changes named with exit 1, an excerpt deletion and other
paths exit 0, the CI workflow calls it -> test_range; random staging and commits
against a model of HEAD, index and working tree -> test_commit_random;
resume.py precompact (expected lines written here from the hook's specification): the target from the newest typed
/package, /prune or /finish command (first argument; /finish without one keeps the earlier target), never from tool
results, user text blocks, assistant text, system or attachment records quoting one, a broken line or other commands;
else from the branch name, with no skill line; the skill line; the /finish ledger line (not written yet, existing, the
typed target's and the branch's both, an existing ledger without a package, none without either); the payload's cwd,
else the working directory; the 9000-character cap; exit 0 and no stderr on garbage, empty, binary, non-object and
wrong-typed stdin; the compact mode reading the same target and ledger, injecting only well-formed open rows (gaps.py
ledger_rows: a done row and a note are not rows, blank lines not counted) with '(N closed rows[ and M lines that are not
rows] not shown; open rows)', '(none)' without open rows, 40 rows then '... k more in the ledger' -> test_precompact;
the real gaps.target() and pause.py (SUP34), on committed copies of the real resume.py, gaps.py, pause.py, plan.py and
worktree.py over a plan of P900 (SUP32, SUP33) and P018 (C18): target() with a batch id winning over a package id, arg
before the branch and worktree names, a warning for an arg naming neither, a branch with a slash as key, detached HEAD,
a linked worktree's folder name, a claim branch Pxxx-<batch>-<user> taking its first batch the finish log has not landed
(all landed: the branch name), a typed id kept -> test_gaps_target; pause.py's key from the transcript under
~/.claude/projects/*/<session>.jsonl: typed /package, /prune and /finish with batch and package ids, /finish without one
keeping the earlier, quoted args, quoted commands, other commands, broken, non-object and binary lines, none, no
transcript, an arg naming nothing, a linked worktree on a claim branch with and without a typed command -> test_pause_key;
pause.py's longest output (24 hours, 28 pings) within plan.FLOOR, so it needs no fitting, and 24.01 hours refused ->
test_pause_bound;
notify.py: title from the newest custom-title record (compact or spaced JSON, escapes, whitespace stripped), not from a
tool result quoting one, also one 2 MiB before the end; fallback '<checkout folder> (<branch>)' for a
missing, title-less or wrong-typed transcript, a subfolder, another branch, detached HEAD (folder only), a non-git
folder, a missing or wrong-typed cwd (the working directory); why-text per notification_type with and without message;
header and body cuts at 120 and 300 characters; garbage stdin still notifies 'needs attention'; no notify-send on PATH,
notify-send failing or hanging -> exit 0, no output -> test_notify; in VS Code's terminal on Linux (fake notify-send
answering --help with NOTIFY_HELP, held by NOTIFY_GATE, logging its session id and printing NOTIFY_ACTION; fake code
logging its argv, token variables and which session variables reached it), the hook
returns before the helper notifies, the helper runs in its own session without CP_REAP_KEEP (so /reap --own ends it) and a default action, a click
runs code (or an editor's own launcher found from VSCODE_GIT_ASKPASS_MAIN's product.json, in either bin folder, its
storage folder named by nameShort) on the
deepest open window folder of storage.json holding cwd (deeper listed first or last, a percent-encoded space, a
sibling sharing a name prefix; unreadable or odd storage: the main checkout, also from a linked worktree), inherited
activation tokens and session variables dropped (VSCODE_IPC_HOOK_CLI kept), a dismissal runs nothing, on KDE the KWin
activation before code runs; the editor found from the hook's ancestors (a copy of the shell as a fake editor beside
its product.json) with git.enabled off and before the askpass script, its --user-data-dir window list, its own launcher,
its pid in the KWin script and the terminal-focus URI opened after code only when the extension's folder with a
package.json sits in its extensions folder; a product.json without applicationName runs no launcher at all; outside VS
Code, without code or without --action the plain notification -> test_notify_click; host() over a fake /proc
(TERM_PROGRAM ancestors, both product.json layouts, the main process, --user-data-dir and --extensions-dir in both
command line forms, an outside ancestor, missing entries, loops, pid 1, no /proc), option(), editor() preferring the
walk's product without falling back, extensions() and focuser() -> test_notify_host; script() run by Node.js on fake
windows (pid over full path over name, ~/ under home, the app suffix, normal windows, ties, activeClient, the probe's
printed counts) -> test_notify_kwin_script; the terminal-focus extension's package.json and its URI handler under
Node.js with a stub vscode module -> test_notify_extension; the helper ended by SIGTERM or SIGHUP sends SIGINT to
notify-send and exits 0 without a launcher -> test_notify_end; .vscode/settings.json's window.title against VS Code's
default (and the installed bundle) and tracked -> test_notify_title_setting; untitled workspaces under the user data
folder and window folders compared through symbolic links -> test_notify_folders; activate() (gdbus, qdbus6, qdbus argv; the id loadScript returns, none on an error; the script file
removed; folder name and app as JSON; nothing outside KDE, without an app or a D-Bus tool) -> test_notify_platforms; command() with stubbed notifiers: macOS terminal-notifier with and without the
`open -b` click, osascript without one, none; WSL and Windows PowerShell toasts decoded (protocol launch URI with distro
and quoting, an ASCII-only XML literal without quote marks for any title, no click without focus or distro), the fixed
powershell.exe path from WSL, no PowerShell; detect() on WSL kernels and variables; the helper giving up after WAIT;
editor() on a missing, short, unreadable, non-object or partly wrong-typed product.json; cli() in either bin folder,
executable only, else PATH; folders() on Windows (drive-letter file URIs, no APPDATA) and WSL (this distribution's
vscode-remote URIs, any case, from Windows' storage of every user, a glob-special editor name, no distribution name),
window() on WSL; title() at every chunk size from 1 byte, across boundaries, without a final newline, non-ASCII,
and its stop RESERVE=3 seconds before DEADLINE ('' with a title present when the deadline is near or past, or passes
mid-search on a stepped clock; found while live); storage() (none without a name; VSCODE_PORTABLE, else
VSCODE_APPDATA, else XDG_CONFIG_HOME or ~/.config on Linux, ~/Library/Application Support on macOS, APPDATA on Windows;
on WSL every Windows user's roaming folder, the variables ignored); local() (file URIs on Linux and Windows, this
distribution's vscode-remote URIs on WSL in any case, others None); workspace() on Linux, Windows (PureWindowsPath,
drive kept, backslashes as slashes) and WSL (line and block comments, trailing commas, strings holding // and /* kept;
relative paths resolved and normalized, absolute kept, uri entries through local(), others skipped); folders() on
workspace windows (a missing, malformed, non-UTF-8, list or wrong-typed file or entry skips that window only; a folder
entry wins) and window() picking the deepest folder of a workspace window -> test_notify_platforms;
guard.py refuses Edit, Write and NotebookEdit under any .git (config, hooks, a worktree's .git file, another repository,
relative, through a symlink, any case), reads and look-alike names pass -> test_guard_git; pre refuses .git/config,
.git/hooks and worktree config paths in any case and with repeated slashes or /./, and git config includes -> test_pre_push.
"""
import argparse
import base64
import contextlib
import importlib.util
import json
import os
from pathlib import Path
import random
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import types
import unittest
import unittest.mock
from urllib.parse import quote
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
r = f + '.reattach'
print(json.dumps({'errors': bad, 'skill_reattach': [l for l in open(r).read().splitlines() if l] if os.path.exists(r) else []}))
sys.exit(1 if bad else 0)
'''
WORKSPACE = '''import pathlib, sys
p = pathlib.Path(__file__).resolve().parents[1] / '99-Workspace/template.cpp'
sys.exit(0 if sys.argv[1:] == ['--check'] and p.is_file() and p.read_text() == 'generated\\n' else 1)
'''
GAPS = '''import re, subprocess
BATCHES, PACKAGES = {'SUP32': 'P900'}, ('P900', 'P018')


def target(root, arg=''):
    """The real return shape: (key, package dict, batch ids, warning when arg names no package); a batch id wins, arg before the branch."""
    branch = subprocess.run(['git', 'branch', '--show-current'], cwd=root, capture_output=True, text=True).stdout.strip()

    def find(name):
        tokens = [t.upper() for t in re.findall(r'[A-Za-z]+\\d+', name)]
        batch = next((t for t in tokens if t in BATCHES), None)
        return batch, BATCHES[batch] if batch else next((t for t in tokens if t in PACKAGES), None)
    batch, pid = find(arg)
    warning = f'no package or batch id in "{arg}": the branch and worktree names decide' if arg and not pid else ''
    if not pid:
        batch, pid = find(branch + ' ' + root.name)
    key = batch or pid or re.sub(r'[^\\w.-]', '_', branch or 'detached')
    pkg = {'id': pid, 'batches': [b for b, p in BATCHES.items() if p == pid]} if pid else None
    return key, pkg, [batch] if batch else pkg['batches'] if pkg else [], warning
'''
GAPS_ROWS = '''
import importlib.util
_spec = importlib.util.spec_from_file_location('real_finish_gaps', {path!r})
_real = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_real)
ledger_rows = _real.ledger_rows  # the real ledger rule (SUP33), which resume.py's compact mode applies
'''
FAKE_NOTIFY = '''#!/bin/sh
if [ "$1" = --help ]; then printf '%s\\n' "${NOTIFY_HELP---action}"; exit 0; fi
i=0
while [ -n "$NOTIFY_GATE" ] && [ ! -e "$NOTIFY_GATE" ] && [ $i -lt 400 ]; do sleep 0.05; i=$((i + 1)); done
printf '%s\\0' "$@" >> "$NOTIFY_LOG"
printf '\\036' >> "$NOTIFY_LOG"
[ -n "$NOTIFY_SID" ] && printf '%s %s' "$(ps -o sid= -p $$ | tr -d ' ')" "${CP_REAP_KEEP:-none}" > "$NOTIFY_SID"
[ -n "$NOTIFY_SLEEP" ] && exec sleep "$NOTIFY_SLEEP"
case " $* " in *" --action=default=Open "*) printf '%s\\n' "${NOTIFY_ACTION-default}";; esac
exit "${NOTIFY_EXIT:-0}"
'''
FAKE_CODE = '''#!/bin/sh
case " $* " in *" --open-url "*) printf '%s\\0' "$@" > "$CODE_LOG.url"; exit 0;; esac
{
printf '%s\\0' "$@" "token=$XDG_ACTIVATION_TOKEN" "startup=$DESKTOP_STARTUP_ID"
for v in CLAUDECODE CLAUDE_PID CP_USER CP_REAP_KEEP AI_AGENT VSCODE_GIT_IPC_HANDLE VSCODE_IPC_HOOK_CLI; do eval "[ -n \\"\\${$v+x}\\" ]" && printf '%s\\0' $v; done
} > "$CODE_LOG.part"
mv "$CODE_LOG.part" "$CODE_LOG"
'''
FAKE_DBUS = '''#!/bin/sh
for a; do printf '%s\\0' "$a"; done >> "$KWIN_LOG"
printf '\\n' >> "$KWIN_LOG"
for a; do case "$a" in *.js) while IFS= read -r l; do printf '%s\\n' "$l"; done < "$a" > "$KWIN_LOG.js"; [ -e "$CODE_LOG" ] && echo late >> "$KWIN_LOG.order";; esac; done
case "$*" in *loadScript*) printf '%s\\n' "$KWIN_OUT";; esac
'''
PLAN_DIR = '00-Guidelines/13-Plan/'
FINISH_HEAD = 'date\tkey\tmodel\teffort\treview_findings\tdefects_fixed\tdecisions\tresult\texperiment\n'
TARGET_DRIVER = '''import importlib.util, json, sys
from pathlib import Path
spec = importlib.util.spec_from_file_location('finish_gaps', sys.argv[1])
gaps = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gaps)
for root, arg in json.loads(sys.argv[2]):
    key, pkg, batches, warning = gaps.target(Path(root), arg)
    print(json.dumps([key, pkg and pkg['id'], batches, warning]))
'''
SH = os.path.realpath('/bin/sh')  # the notify runs' parent shell (a real file: a copy fakes an editor)
NODE = shutil.which('node') or shutil.which('nodejs')  # runs the KWin script and the extension; their cases skip without it
DEFAULT_TITLE = '${dirty}${activeEditorShort}${separator}${rootName}${separator}${profileName}${separator}${appName}'  # VS Code 1.140, Linux
TOOLS = ('git', 'mv', 'ps', 'tr', 'sleep')  # what the fakes and notify.py run besides themselves, without the real code
SESSION_VARS = {v: 'x' for v in ('CLAUDECODE', 'CLAUDE_PID', 'CP_USER', 'CP_REAP_KEEP', 'AI_AGENT', 'VSCODE_GIT_IPC_HANDLE',
                                 'VSCODE_IPC_HOOK_CLI')}
PRE_HEAD = 'Compaction instructions from .claude/hooks/resume.py (CLAUDE.md, Compact Instructions).'
PRE_KEEP = ('Keep the package or batch id, the skill and step in progress, decisions made and by whom, operations done and '
            'remaining, failing reproducers with seeds, and the exact next step. Drop file contents, passing test output and '
            'superseded attempts.')


def typed(skill, args=''):
    """A user record of a typed slash command, as Claude Code writes it."""
    return {'type': 'user', 'message': {'role': 'user', 'content': f'<command-message>{skill}</command-message>\n'
                                        f'<command-name>/{skill}</command-name>\n<command-args>{args}</command-args>'}}


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
        self.log, self.fail_file = self.tmp / 'log', self.tmp / 'fail'
        self.env = {k: v for k, v in os.environ.items() if not k.startswith('GIT_')} | {
            'HOOK_LOG': str(self.log), 'HOOK_FAIL': str(self.fail_file), 'GIT_CONFIG_GLOBAL': os.devnull, 'GIT_CONFIG_NOSYSTEM': '1'}
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

    def command(self, *ops, tid=None, event=None):
        """(hook output or None, whether the validator ran) of one Bash command made of ops; event: post's hook_event_name."""
        self.n += 1
        payload = {'session_id': 's1', 'tool_use_id': tid or f'toolu_{self.n}', 'tool_name': 'Bash', 'tool_input': {'command': 'x'}}
        self.log.unlink(missing_ok=True)
        self.hook('shell.py', 'pre', payload=payload)
        for op in ops:
            op()
        out = self.hook('shell.py', 'post', payload=payload | ({'hook_event_name': event} if event is not None else {}))
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
        self.fail_file.write_text('Missing row: x\nBad link: y\n')
        out, ran = self.command(self.edit('05-M/00-index.md', 'v2\n'))
        self.assertTrue(ran)
        self.assertEqual(out, {'hookSpecificOutput': {'hookEventName': 'PostToolUse',
                                                      'additionalContext': 'Consistency validator FAILED:\nMissing row: x\nBad link: y'}})

    def test_block_and_context(self):
        self.fail_file.write_text('E\n')
        out, ran = self.command(self.edit('OLD/a.cpp', 'c\n'), self.edit('05-M/00-index.md', 'c\n'))
        self.assertTrue(ran and self.blocked(out))
        self.assertEqual(out['hookSpecificOutput']['additionalContext'], 'Consistency validator FAILED:\nE')

    def test_event_name(self):
        """The validator's additionalContext names the input's hook_event_name, PostToolUse when it has none (SUP33)."""
        self.fail_file.write_text('E\n')
        for n, (event, name) in enumerate((('PostToolUseFailure', 'PostToolUseFailure'), ('PostToolUse', 'PostToolUse'), ('', 'PostToolUse'),
                                           (None, 'PostToolUse'))):
            out, ran = self.command(self.edit('05-M/00-index.md', f'e{n}\n'), event=event)
            self.assertEqual((out, ran), ({'hookSpecificOutput': {'hookEventName': name, 'additionalContext': 'Consistency validator FAILED:\nE'}},
                                          True), event)

    def test_hooks_path(self):
        """pre stores core.hooksPath beside HEAD and paths; post blocks when the command changed it and git no longer runs
        this checkout's .claude/githooks (setup_check.py, hook_check), naming the restore command, also beside a protected
        file's block; another spelling of the same absolute path, no change, and an old two-element state pass (SUP33)."""
        self.write('.claude/skills/setup-check/setup_check.py', (ROOT / '.claude/skills/setup-check/setup_check.py').read_text())
        key, want = 'core.hooksPath', str(self.repo / '.claude/githooks')
        quoted = want if re.fullmatch(r'[\w@%+=:,./-]+', want) else "'" + want.replace("'", "'\"'\"'") + "'"  # POSIX shell quoting

        def lost(state):
            return {'decision': 'block', 'reason': f'This command changed {key} ({state}), so git no longer runs the commit and push hooks '
                    f'(CLAUDE.md, Start Here). Ask the user to restore it by typing `! git config {key} {quoted}`; a push the same command '
                    'made has already run.'}

        def cfg(*args):
            return self.gitop('config', *args)
        state = self.repo / '.claude/hooks/.state/toolu_h.json'
        payload = {'session_id': 's', 'tool_use_id': 'toolu_h', 'tool_name': 'Bash', 'tool_input': {'command': 'x'}}
        for value in (None, want):
            if value:
                git(self.repo, 'config', key, value)
            self.hook('shell.py', 'pre', payload=payload)
            snap = json.loads(state.read_text())
            self.assertEqual((len(snap), snap[0], snap[2]), (3, git(self.repo, 'rev-parse', 'HEAD').strip(), value), 'pre: [HEAD, paths, hooks]')
            self.assertIsNone(self.hook('shell.py', 'post', payload=payload))
        self.assertEqual(self.command(cfg('--unset', key)), (lost('not enabled'), False), 'unset')
        self.assertEqual(self.command(), (None, False), 'unchanged, though off')
        self.assertEqual(self.command(cfg(key, '.claude/githooks')), (lost(f"{key} is '.claude/githooks'"), False), 'relative')
        self.assertEqual(self.command(cfg(key, want)), (None, False), 'restored')
        for spelling in (str(self.repo) + '/./.claude/githooks', str(self.repo / '.claude/../.claude/githooks'), want + '/', want):
            self.assertEqual(self.command(cfg(key, spelling)), (None, False), f'another spelling of the same path: {spelling}')
        other = str(self.tmp / 'elsewhere')
        self.assertEqual(self.command(cfg(key, other)), (lost(f"{key} is '{other}'"), False), 'another folder')
        self.assertEqual(self.command(self.edit('notes.txt', 'n2\n')), (None, False), 'unchanged, though wrong')
        git(self.repo, 'config', key, want)
        self.fail_file.write_text('E\n')
        out, ran = self.command(self.edit('OLD/a.cpp', 'c\n'), cfg('--unset', key), self.edit('05-M/00-index.md', 'c\n'))
        reason = self.blocked(out)
        self.assertTrue(ran and reason.startswith('This command changed protected files: OLD/a.cpp') and reason.endswith(' ' + lost('not enabled')['reason'])
                        and reason.count('This command changed') == 2, reason)
        self.assertEqual(out['hookSpecificOutput']['additionalContext'], 'Consistency validator FAILED:\nE', 'block and validator together')
        self.fail_file.unlink()
        git(self.repo, 'checkout', '--', 'OLD/a.cpp')
        git(self.repo, 'config', key, want)
        self.hook('shell.py', 'pre', payload=payload)
        state.write_text(json.dumps(json.loads(state.read_text())[:2]))
        git(self.repo, 'config', '--unset', key)
        self.assertIsNone(self.hook('shell.py', 'post', payload=payload), 'a two-element state from before SUP33: no hooks check')
        self.assertFalse(state.exists())

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

    def test_consistency_batch(self):
        def batch(*calls):
            return {'hook_event_name': 'PostToolBatch', 'tool_calls': [
                {'tool_name': tool, 'tool_input': {'file_path': p if p.startswith('/') else str(self.repo / p)}, 'tool_use_id': f'toolu_{k}',
                 'tool_response': ''} for k, (tool, p) in enumerate(calls)]}

        def runs(payload=None, raw=None, out=None):
            self.log.unlink(missing_ok=True)
            self.assertEqual(self.hook('consistency.py', payload=payload, raw=raw), out, (payload, raw))
            return self.log.read_text().count('validate') if self.log.exists() else 0
        outside, escaping = str(self.tmp / 'outside.md'), str(self.repo / '..' / 'outside.md')  # the second leaves the root only once resolved
        for calls, n in (((('Edit', '05-M/00-index.md'),), 1), ((('Write', '05-M/01-a.hpp'),), 1),
                         ((('Edit', '00-Guidelines/13-Plan/packages.json'),), 1), ((('Write', '08-Python/_01_a.py'),), 1),
                         ((('Edit', 'notes.txt'), ('Write', 'tools/t.py'), ('Edit', outside), ('Write', escaping)), 0),
                         ((('Read', '05-M/00-index.md'), ('NotebookEdit', '05-M/00-index.md'), ('Bash', '05-M/01-a.hpp')), 0),
                         ((('Edit', 'notes.txt'), ('Edit', '05-M/00-index.md'), ('Write', '05-M/02-b.md'), ('Edit', '05-M/01-a.hpp')), 1)):
            self.assertEqual(runs(batch(*calls)), n, calls)
        relative = {'tool_calls': [{'tool_name': 'Edit', 'tool_input': {'file_path': '05-M/00-index.md'}}]}
        self.assertEqual(runs(relative), 1, 'a relative path is under the root')
        malformed = [None, 'Edit', {'tool_name': 'Edit'}, {'tool_name': 'Edit', 'tool_input': {'file_path': 5}}, {'tool_input': {'file_path': 'x.md'}}]
        self.assertEqual(runs({'tool_calls': malformed + batch(('Edit', '05-M/00-index.md'))['tool_calls']}), 1, 'malformed calls skipped')
        self.assertEqual((runs({'tool_calls': malformed}), runs({'tool_calls': []})), (0, 0))
        notice = {'hookSpecificOutput': {'hookEventName': 'PostToolBatch', 'additionalContext': (
            "Consistency hook: its input carries no tool_calls list, so the validator did not run; run python3 "
            "'96-Local Testing/03-consistency.py' and check the PostToolBatch input (.claude/hooks/consistency.py).")}}
        for payload in ({'tool_calls': {'tool_name': 'Edit', 'tool_input': {'file_path': '05-M/00-index.md'}}}, {'tool_calls': 5},
                        {'tool_calls': None}, {'tool_calls': '05-M/00-index.md'}, {'tool_call': []}, {}, [], 'x', 5,
                        {'tool_input': {'file_path': '05-M/00-index.md'}}):  # the last is a PostToolUse payload
            self.assertEqual(runs(payload, out=notice), 0, payload)
        for raw in ('', '[', '{"tool_calls": [', 'not json'):  # unreadable input: silent
            self.assertEqual(runs(raw=raw), 0, raw)
        self.fail_file.write_text('F\n')
        failed = 'Consistency validator FAILED:\nF'
        self.assertEqual(self.hook('consistency.py', payload=batch(('Edit', 'notes.txt'), ('Edit', '05-M/00-index.md'))),
                         {'hookSpecificOutput': {'hookEventName': 'PostToolBatch', 'additionalContext': failed}})
        # skill_reattach warnings (SUP33) reach the context only from a batch that edited a skill's SKILL.md, and never fail it
        reattach = Path(str(self.fail_file) + '.reattach')
        reattach.write_text('finish: 9000 of 12500 characters\nx: y\n')
        warn = "Skill re-attachment warning (SUP33; 03-consistency.py, skill_reattach):\nfinish: 9000 of 12500 characters\nx: y"
        ctx = lambda text: text and {'hookSpecificOutput': {'hookEventName': 'PostToolBatch', 'additionalContext': text}}
        for fail, paths, want in ((True, ['.claude/skills/finish/SKILL.md'], failed + '\n\n' + warn), (True, ['05-M/00-index.md'], failed),
                                  (False, ['.claude/skills/finish/SKILL.md', 'notes.txt'], warn), (False, ['05-M/00-index.md'], None),
                                  (False, ['.claude/skills/finish/notes.md'], None), (False, ['.claude/SKILL.md'], None),
                                  (False, ['.claude/skills/a/b/SKILL.md'], warn)):  # the last: any depth below .claude/skills/
            if fail:
                self.fail_file.write_text('F\n')
            else:
                self.fail_file.unlink(missing_ok=True)
            self.assertEqual(self.hook('consistency.py', payload=batch(*[('Edit', p) for p in paths])), ctx(want), (fail, paths))
        reattach.write_text('')
        self.assertIsNone(self.hook('consistency.py', payload=batch(('Edit', '.claude/skills/finish/SKILL.md'))), 'no warnings: silent')

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

    def test_submit(self):
        deny = {'hookSpecificOutput': {'hookEventName': 'PreToolUse', 'permissionDecision': 'deny', 'permissionDecisionReason':
                                       "Submitting is the user's (01-principles.md, Scope Discipline): ask them to run 04-submit.py themselves, "
                                       "for example with `! CP_USER=1 python3 '97-Online Testing/04-submit.py'`."}}
        state = self.repo / '.claude/hooks/.state'
        script = "'97-Online Testing/04-submit.py'"
        for cmd in (f'python3 {script}', f'python3 {script} list', f'python3 -I -u {script} all --yes', f'python3 -X dev {script}',
                    f'/usr/bin/python3.14 {script} login', f'pypy3 {script}', f'cd x && python3 {script}', f'env A=1 python3 {script}',
                    f'CP=1 python3 {script}', './04-submit.py stale', f'"/abs/97-Online Testing/04-submit.py"', f'true; python {script}',
                    f"cat <<'EOF'\nx\nEOF\npython3 {script}", f'(python3 {script})'):
            payload = {'session_id': 's', 'tool_use_id': 'toolu_s', 'tool_name': 'Bash', 'tool_input': {'command': cmd}}
            self.assertEqual(self.hook('shell.py', 'pre', payload=payload), deny, cmd)
            self.assertFalse((state / 'toolu_s.json').exists(), cmd)
        for cmd in (f'cat {script}', f'git add {script}', f'python3 -m py_compile {script}', f'sed -n 1,5p {script}',
                    "python3 '96-Local Testing/00-Tools/16-submit_tester.py'", f'echo python3 {script}', f'grep -n submit {script}',
                    f"python3 - <<'EOF'\npython3 {script}\nEOF", 'python3 03-submitted.py', f'python3 -c "import x" {script}'):
            payload = {'session_id': 's', 'tool_use_id': 'toolu_t', 'tool_name': 'Bash', 'tool_input': {'command': cmd}}
            self.assertIsNone(self.hook('shell.py', 'pre', payload=payload), cmd)
            self.assertIsNone(self.hook('shell.py', 'post', payload=payload), cmd)

    def test_memory_cap_note(self):
        """SUP34: `pre` adds a non-blocking note naming _00_memory_cap.py to a command that compiles or runs a built binary in
        command position, and to nothing else: text that mentions a compiler, a compiler query, a wrapped command, a tester
        or another script; a runner's options with their values, perf's subcommand and setarch's architecture are skipped, and
        the named word is cut to 80 characters; the snapshot is still taken, and a Monitor command gets the note too."""
        text = ("This command {} ({}) outside the memory cap. Run compiles, scratch binaries and probes as "
                "`python3 '96-Local Testing/_00_memory_cap.py' -- <command>` (CLAUDE.md, Commands; 05-testing.md, Memory Safety), "
                "so a runaway process dies alone and the memory gate admits it beside other sessions' runs.")
        note = lambda what, word: {'hookSpecificOutput': {'hookEventName': 'PreToolUse', 'additionalContext': text.format(what, word)}}
        elf, script, folder = self.tmp / 'prog', self.tmp / 'tool', self.tmp / 'dir'
        shutil.copy('/bin/true', elf)
        script.write_text('#!/bin/sh\necho x\n')
        script.chmod(0o755)
        folder.mkdir()
        os.mkfifo(self.tmp / 'fifo')  # opening it would block the hook
        state = self.repo / '.claude/hooks/.state'
        cap = "python3 '96-Local Testing/_00_memory_cap.py' --"
        compiles = (('g++ -O2 a.cpp -o a', 'g++'), ('g++-14 -std=gnu++20 x.cpp', 'g++-14'), ('cd d && clang++ a.cpp', 'clang++'),
                    ('ccache g++ a.cpp', 'g++'), ('timeout 10 g++ a.cpp', 'g++'), ('/usr/bin/g++-15 a.cpp', '/usr/bin/g++-15'),
                    ('x86_64-linux-gnu-gcc-14 -c a.c', 'x86_64-linux-gnu-gcc-14'), ('cc a.c', 'cc'), ('c++ a.cpp', 'c++'),
                    ('nice -n 5 gcc a.c', 'gcc'), (f'{cap} ./a.out; g++ b.cpp', 'g++'), ('g++ --version && g++ a.cpp', 'g++'),
                    ('g++ -v a.cpp', 'g++'), ('/home/' + 'd' * 100 + '/g++ a.cpp', '/home/' + 'd' * 74))
        runs = ((f'{elf} < in', str(elf)), ('./a.out', './a.out'), ('timeout 5s ./sol', './sol'), ('A=1 ./missing/bin', './missing/bin'),
                (f'taskset 0x1 {elf}', str(elf)), (f'echo x | "{elf}"', str(elf)), ('timeout -s KILL 10 ./a.out', './a.out'),
                ('timeout --signal KILL -k 2 5 ./a.out', './a.out'), ('perf stat -e cycles ./a.out', './a.out'), ('perf stat -r 5 ./a.out', './a.out'),
                ('setarch x86_64 -R ./a.out', './a.out'), ('taskset -c 0-3 ./a.out', './a.out'), ('./' + 'x' * 120, './' + 'x' * 78))
        quiet = ('grep -n g++ file', 'git log --grep=gcc', 'echo g++ a.cpp', "rg 'clang\\+\\+' x", 'g++ --version', 'gcc -dumpversion',
                 'g++ -print-file-name=plugin', 'clang++ -v', f'{cap} g++ a.cpp', f'{cap} ./a.out', f'{cap} {elf}',
                 "python3 '96-Local Testing/00-Tools/05-runner_tester.py' --mode full", "'96-Local Testing/00-Tools/05-runner_tester.py'",
                 'which g++', 'clang-format -i a.cpp', 'ls ./build', './script.sh', '.venv/bin/python3 x.py', str(script), str(folder),
                 '/usr/bin/ls', '/dev/stdin', str(self.tmp / 'fifo'), 'cat <<EOF\ng++ a.cpp\nEOF', 'make', 'gcc-ar rcs a.a', 'git commit -m "g++ a.cpp"', 'ls -l prog')
        for cmd, want in [*((c, note('compiles', w)) for c, w in compiles), *((c, note('runs a built binary', w)) for c, w in runs),
                          *((c, None) for c in quiet)]:
            state.mkdir(parents=True, exist_ok=True)
            for old in state.glob('*.json'): old.unlink()
            payload = {'session_id': 's', 'tool_use_id': 'toolu_m', 'tool_name': 'Bash', 'tool_input': {'command': cmd}}
            self.assertEqual(self.hook('shell.py', 'pre', payload=payload), want, cmd)
            self.assertTrue((state / 'toolu_m.json').exists(), cmd)
        payload = {'session_id': 's', 'tool_use_id': 'toolu_n', 'tool_name': 'Monitor', 'tool_input': {'command': 'g++ a.cpp'}}
        self.assertEqual(self.hook('shell.py', 'pre', payload=payload), note('compiles', 'g++'))
        self.assertFalse((state / 'toolu_n.json').exists())

    def test_pre_push(self):
        """The pre-push hook (`shell.py push`) end to end through git, and `pre` on commands naming CP_USER (SUP32)."""
        self.write('.claude/githooks/pre-push', (ROOT / '.claude/githooks/pre-push').read_text())
        (self.repo / '.claude/githooks/pre-push').chmod(0o755)
        git(self.repo, 'config', 'core.hooksPath', str(self.repo / '.claude/githooks'))
        remote = self.tmp / 'remote.git'
        subprocess.run(['git', 'init', '-q', '--bare', str(remote)], check=True, capture_output=True)
        base = {k: v for k, v in self.env.items() if k not in ('CLAUDE_CODE_CHILD_SESSION', 'CP_USER', 'CLAUDECODE')}
        refused = ("Push refused by .claude/githooks/pre-push: this shell was started by Claude Code, and pushing is the user's "
                   '(CLAUDE.md, Git). Typed yourself with `!`? Run it as `CP_USER=1 git push ...`.\n')
        forms = (['git', 'push'], ['sh', '-c', 'git push "$@"', 'sh'], ['sh', '-c', 'g=git; $g push "$@"', 'sh'])
        cases = (({}, True), ({'CLAUDE_CODE_CHILD_SESSION': '1'}, False), ({'CLAUDE_CODE_CHILD_SESSION': '1', 'CP_USER': '1'}, True),
                 ({'CLAUDE_CODE_CHILD_SESSION': '0'}, True), ({'CLAUDE_CODE_CHILD_SESSION': '1', 'CP_USER': '0'}, False),
                 ({'CLAUDE_CODE_CHILD_SESSION': '1', 'CP_USER': ''}, False), ({'CLAUDECODE': '1'}, True))
        expect = set()
        for n, (extra, ok) in enumerate(cases):
            for k, form in enumerate(forms):
                ref = f'b{n}-{k}'
                r = subprocess.run([*form, '-q', str(remote), f'HEAD:refs/heads/{ref}'], cwd=self.repo, env=base | extra,
                                   capture_output=True, text=True, timeout=60)
                self.assertEqual(r.returncode == 0, ok, (extra, form, r.stderr))
                self.assertEqual(r.stderr.startswith(refused), not ok, (extra, form, r.stderr))
                expect |= {ref} if ok else set()
        have = set(git(self.repo, '-C', str(remote), 'for-each-ref', '--format=%(refname:short)').split())
        self.assertEqual(have, expect, 'refused pushes leave the remote untouched')
        def deny(text):
            return {'hookSpecificOutput': {'hookEventName': 'PreToolUse', 'permissionDecision': 'deny', 'permissionDecisionReason':
                    f'This command names {text!r}, which would get past the push and submit guards or the git hooks (CLAUDE.md, Git '
                    "and Start Here): pushing and submitting are the user's, and the hooks are never bypassed. Read, search or edit "
                    'such text with the Read, Grep or Edit tools.'}}
        bypass = (('CP_USER=1 git status', 'CP_USER'), ('export CP_USER=1', 'CP_USER'), ('echo $CP_USER', 'CP_USER'),
                  ("sh -c 'CP_USER=1 git push'", 'CP_USER'), ('env CP_USER=1 make', 'CP_USER'), ("cat <<'EOF'\nCP_USER\nEOF", 'CP_USER'),
                  ('env -u CLAUDE_CODE_CHILD_SESSION git status', 'CLAUDE_CODE_CHILD_SESSION'),
                  ("sh -c 'unset CLAUDE_CODE_CHILD_SESSION; ls'", 'CLAUDE_CODE_CHILD_SESSION'),
                  ('git commit -q --no-verify -m x', '--no-verify'), ("sh -c 'git push --no-verify o b'", '--no-verify'),
                  ('git -c core.hooksPath=/x push', 'core.hooksPath='), ("sh -c 'git -c core.hookspath = /x push'", 'core.hookspath ='),
                  ('git config core.hooksPath /x', 'core.hooksPath /'), ("GIT_CONFIG_PARAMETERS=\"'core.hooksPath'='/x'\" ls", "core.hooksPath'="),
                  ('GIT_CONFIG_COUNT=1 GIT_CONFIG_KEY_0=core.hooksPath GIT_CONFIG_VALUE_0=/x ls', 'core.hooksPath G'),
                  ('git config --unset core.hooksPath', '--unset core.hooksPath'), ('git config unset core.hookspath', 'unset core.hookspath'),
                  ('git config --unset-all core.hooksPath', '--unset-all core.hooksPath'), ('git config --remove-section core', '--remove-section core'),
                  ("env -i sh -c 'git push o b'", 'env -i'), ('env -u X -i git status', 'env -u X -i'), ('env --ignore-environment ls', 'env --ignore-environment'),
                  ('env - PATH=/bin sh', 'env -'), ('env A=1 -i ls', 'env A=1 -i'), ('env -i git' + ' push', 'env -i'),
                  ("sed -i 's/x/y/' .git/config", '.git/config'), ('cat > repo/.git/config.worktree', '.git/config'),
                  ('rm .git/hooks/pre-push', '.git/hooks'), ("printf x > '.git/hooks/pre-commit'", '.git/hooks'),
                  ('cat .git//config', '.git//config'), ('rm .git/./hooks/x', '.git/./hooks'), ('vi .GIT/Config', '.GIT/Config'),
                  ('ed .git/worktrees/wt/config.worktree', '.git/worktrees/wt/config'),
                  ('ed x.git//.//worktrees//a//config', '.git//.//worktrees//a//config'),
                  ('git config include.path ../x', 'include.path'), ('git config includeIf.gitdir:/x/.path y', 'includeIf.'))
        for tool in ('Bash', 'Monitor'):
            for cmd, text in bypass:
                payload = {'session_id': 's', 'tool_use_id': 'toolu_u', 'tool_name': tool, 'tool_input': {'command': cmd}}
                self.assertEqual(self.hook('shell.py', 'pre', payload=payload), deny(text), (tool, cmd))
                self.assertFalse((self.repo / '.claude/hooks/.state/toolu_u.json').exists(), cmd)
        for cmd in ('echo CP_USERS', 'echo XCP_USER', 'echo MY_CP_USER_2', 'git config core.hooksPath', 'git config --get core.hooksPath; ls',
                    'echo --no-verifyx', 'echo CLAUDE_CODE_CHILD_SESSIONS', 'env A=1 grep -i x f', 'environ -i', 'env -u X ls',
                    'git config --get-regexp core', 'ls .github/workflows', 'cat .gitconfig.bak', 'ls .git/refs', 'ls .git/.hooks',
                    'ls .git/worktrees/wt/HEAD', 'echo included.path', 'ls .gitconfig', 'cat src/config'):
            payload = {'session_id': 's', 'tool_use_id': 'toolu_v', 'tool_name': 'Bash', 'tool_input': {'command': cmd}}
            self.assertIsNone(self.hook('shell.py', 'pre', payload=payload), cmd)
            self.assertIsNone(self.hook('shell.py', 'post', payload=payload), cmd)
        push = {'hookSpecificOutput': {'hookEventName': 'PreToolUse', 'permissionDecision': 'deny', 'permissionDecisionReason':
                                       "Pushing is the user's (CLAUDE.md, Git): print the push command for them to run instead."}}
        git_push = 'git' + ' push'
        for cmd in ('env -u X ' + git_push, 'env A=1 ' + git_push + ' o b', 'sudo -u root ' + git_push,
                    'time -p ' + git_push, 'env --unset=X ' + git_push, 'env -C /tmp ' + git_push):
            for tool in ('Bash', 'Monitor'):
                payload = {'session_id': 's', 'tool_use_id': 'toolu_w', 'tool_name': tool, 'tool_input': {'command': cmd}}
                self.assertEqual(self.hook('shell.py', 'pre', payload=payload), push, (tool, cmd))
        payload = {'session_id': 's', 'tool_use_id': 'toolu_m', 'tool_name': 'Monitor', 'tool_input': {'command': 'tail -f log'}}
        self.assertIsNone(self.hook('shell.py', 'pre', payload=payload))
        self.assertFalse((self.repo / '.claude/hooks/.state/toolu_m.json').exists(), 'Monitor gets no snapshot: post runs for Bash only')

    def script(self, name, *args, raw='', cwd=None, env=None):
        """(exit code, stdout, stderr) of a hook copy run with raw on stdin."""
        r = subprocess.run([sys.executable, str(self.repo / '.claude/hooks' / name), *args], input=raw.encode() if isinstance(raw, str) else raw,
                           capture_output=True, cwd=cwd or self.repo, env=env or self.env, timeout=60)
        return r.returncode, r.stdout.decode(), r.stderr.decode()

    def transcript(self, *recs, extra=b''):
        p = self.tmp / 'session.jsonl'
        p.write_bytes(''.join(json.dumps(r) + '\n' for r in recs).encode() + extra)
        return p

    def precompact(self, path, **extra):
        payload = {'session_id': 's1', 'hook_event_name': 'PreCompact', 'trigger': 'auto', 'custom_instructions': '',
                   'cwd': str(self.repo), 'transcript_path': str(path)} | extra
        rc, out, err = self.script('resume.py', 'precompact', raw=json.dumps(payload), cwd=self.tmp)
        self.assertEqual((rc, err), (0, ''), payload)
        return out

    def test_precompact(self):
        self.write('.claude/hooks/resume.py', (HOOKS / 'resume.py').read_text())
        self.write('.claude/skills/finish/gaps.py', GAPS + GAPS_ROWS.format(path=str(ROOT / '.claude/skills/finish/gaps.py')))
        step = 'Skill in progress: /{}. State the number of its step in progress and the exact next step.'.format
        sup, p018 = 'Target: SUP32 (package P900, from the typed command /package SUP32).', '/finish ledger: .claude/finish/P018.txt, not written yet.'

        def want(*middle):
            return '\n'.join([PRE_HEAD, *middle, PRE_KEEP])
        quote = '<command-name>/prune</command-name>\n<command-args>P018</command-args>'
        quoted = [{'type': 'user', 'message': {'role': 'user', 'content': [{'type': 'tool_result', 'tool_use_id': 'toolu_1', 'content': quote}]}},
                  {'type': 'user', 'message': {'role': 'user', 'content': [{'type': 'text', 'text': quote}]}},
                  {'type': 'assistant', 'message': {'content': [{'type': 'text', 'text': quote}]}}, {'type': 'system', 'content': quote},
                  {'type': 'attachment', 'attachment': {'type': 'text', 'content': quote}}, {'type': 'assistant', 'message': {'content': quote}}]
        p = self.transcript({'type': 'summary', 'summary': 'x'}, typed('package', 'SUP32'), *quoted,
                            extra=b'{"type": "user", "message": {"content": "<command-name>/prune</command-name><command-args>P018</command-args>"\n')
        self.assertEqual(self.precompact(p), want(sup, step('package'), '/finish ledger: .claude/finish/SUP32.txt, not written yet.'),
                         'typed /package SUP32; quoted commands and a broken line do not count')
        a, b = 'a | defect | 05-M/01-a.hpp:1 | gap | review | open', 'b | decision | notes.txt:1 | choice | user | fixed'
        self.write('.claude/finish/SUP32.txt', f'{a}\n{b}\n')
        have = '/finish ledger: .claude/finish/SUP32.txt (its rows hold the state).'
        self.assertEqual(self.precompact(p), want(sup, step('package'), have))

        def compact(text):
            """The ledger part of compact mode's output for SUP32.txt holding text."""
            self.write('.claude/finish/SUP32.txt', text)
            rc, out, err = self.script('resume.py', 'compact', raw=json.dumps({'cwd': str(self.repo), 'transcript_path': str(p)}), cwd=self.tmp)
            self.assertEqual((rc, err), (0, ''))
            self.assertTrue(out.startswith('Restored after compaction by .claude/hooks/resume.py.') and '\n\ngit: branch master;' in out, out[:300])
            return out.split('\n\n')[1] + '\n\n' + out.split('\n\n')[2].split(';')[0] if 'If /finish' in out else out
        head = 'If /finish was running, continue it from its ledger .claude/finish/SUP32.txt '
        self.assertEqual(compact(f'{a}\n{b}\n'), head + f'(1 closed rows not shown; open rows):\n{a}\n\ngit: branch master',
                         'compact mode reads the same target and ledger')
        rows = ['A1 | defect | f:1 | g | s | open', '', 'note: tried X, 3% slower', '  A2 | decision | f:2 | g | s | answered 2026-11-22 (was: open)',
                'A3 | defect | f:3 | g | s | done', 'A4 | decision | f:4 | g | s | ASK the user  ', 'A5 | defect | f:5 | g | s | Fixed in abc']
        self.assertEqual(compact('\n'.join(rows) + '\n'), head + '(2 closed rows and 2 lines that are not rows not shown; open rows):\n'
                         'A1 | defect | f:1 | g | s | open\nA4 | decision | f:4 | g | s | ASK the user\n\ngit: branch master',
                         'well-formed open rows only; a done row and a note are not rows, blank lines not counted')
        self.assertEqual(compact(f'{b}\n'), head + '(1 closed rows not shown; open rows):\n(none)\n\ngit: branch master')
        self.assertEqual(compact('a | open\nb | done\n\n'), head + '(0 closed rows and 2 lines that are not rows not shown; open rows):\n'
                         '(none)\n\ngit: branch master', 'two-field lines of the old form are not rows')
        many = [f'R{i} | defect | f:{i} | g | s | open' for i in range(45)]
        self.assertEqual(compact('\n'.join(many)), head + '(0 closed rows not shown; open rows):\n' + '\n'.join(many[:40])
                         + '\n... 5 more in the ledger\n\ngit: branch master')
        self.write('.claude/finish/SUP32.txt', f'{a}\n{b}\n')
        p = self.transcript(typed('package', 'SUP32'), typed('prune', ' P018  extra words'))
        self.assertEqual(self.precompact(p), want('Target: P018 (package P018, from the typed command /prune P018).', step('prune'), p018))
        p = self.transcript(typed('package', 'SUP32'), typed('finish'))
        self.assertEqual(self.precompact(p), want('Target: SUP32 (package P900, from the typed command /package SUP32).', step('finish'), have),
                         '/finish without an argument keeps the earlier target and names the command that typed it')
        p = self.transcript(typed('pause', 'SUP32'), typed('package-x', 'P018'), typed('packages', 'P018'), typed('prune-all', 'P018'))
        self.assertEqual(self.precompact(p), want(), 'other commands name no target')
        git(self.repo, 'checkout', '-q', '-b', 'P018-fix')
        self.assertEqual(self.precompact(self.transcript(typed('finish'))),
                         want('Target: P018 (package P018, from the branch or worktree name).', step('finish'), p018))
        self.assertEqual(self.precompact(self.transcript(typed('package', 'foo'))),
                         want('Target: P018 (package P018, from the branch or worktree name).', step('package'), p018),
                         'a typed argument naming no package is not the source')
        p = self.transcript('<command-name>/package</command-name>', {'type': 'user', 'message': '<command-name>/prune</command-name>'},
                            typed('package', 'P018'))
        self.assertEqual(self.precompact(p), want('Target: P018 (package P018, from the typed command /package P018).', step('package'), p018),
                         'records that are no object, or whose message is no object, are skipped')
        self.write('.claude/finish/P018.txt', 'c | open\n')
        self.assertEqual(self.precompact(self.transcript(typed('package', 'SUP32'))),
                         want(sup, step('package'), '/finish ledger: .claude/finish/SUP32.txt, .claude/finish/P018.txt (its rows hold the state).'),
                         'the ledger of the typed target and the one of the branch')
        git(self.repo, 'checkout', '-q', '-b', 'SUP32-work')
        self.assertEqual(self.precompact(self.tmp / 'missing.jsonl'), want('Target: SUP32 (package P900, from the branch or worktree name).', have))
        git(self.repo, 'checkout', '-q', 'master')
        self.assertEqual(self.precompact(self.transcript()), want())
        self.write('.claude/finish/master.txt', 'd | open\n')
        self.assertEqual(self.precompact(self.transcript()), want('/finish ledger: .claude/finish/master.txt (its rows hold the state).'),
                         'an existing ledger without a package')
        (self.repo / '.claude/finish/master.txt').unlink()
        rc, out, err = self.script('resume.py', 'precompact', raw=json.dumps({'transcript_path': str(self.transcript(typed('prune', 'P018')))}),
                                   cwd=self.repo / '05-M')
        self.assertEqual((rc, err, out), (0, '', want('Target: P018 (package P018, from the typed command /prune P018).', step('prune'),
                                                      '/finish ledger: .claude/finish/P018.txt (its rows hold the state).')),
                         'no cwd in the payload: the working directory')
        out = self.precompact(self.transcript(typed('package', 'SUP32-' + 'x' * 20000)))
        self.assertEqual(out, want('Target: SUP32 (package P900, from the typed command ' + ('/package SUP32-' + 'x' * 100)[:80] + ').',
                                   step('package'), have), 'a long typed argument is cut, so every fixed line survives')
        for raw, dict_like in (('not json', True), ('', True), (b'\xff\xfe\x00garbage', True), ('{"transcript_path": 5.5}', True),
                               ('{"transcript_path": ["x"]}', True), ('{"hook_event_name": "PreCompact"', True), ('[]', False), ('null', False),
                               ('5', False), ('{"cwd": 7}', False)):
            rc, out, err = self.script('resume.py', 'precompact', raw=raw)
            self.assertEqual((rc, err), (0, ''), raw)
            self.assertIn(out, (want(),) if dict_like else ('', want()), raw)

    def real_finish(self):
        """Commit copies of the real resume.py, gaps.py, pause.py, plan.py and worktree.py with a plan of P900 (batches SUP32,
        SUP33) and P018 (batch C18) and a finish log landing nothing yet."""
        for rel in ('.claude/hooks/resume.py', '.claude/skills/finish/gaps.py', '.claude/skills/pause/pause.py',
                    '00-Guidelines/13-Plan/plan.py', '00-Guidelines/13-Plan/worktree.py'):
            self.write(rel, (ROOT / rel).read_text())
        self.write(PLAN_DIR + 'packages.json', json.dumps({'packages': [{'id': 'P900', 'batches': ['SUP32', 'SUP33']},
                                                                        {'id': 'P018', 'batches': ['C18']}]}))
        self.write(PLAN_DIR + 'batches.json', json.dumps({'batches': [{'id': b} for b in ('SUP32', 'SUP33', 'C18')]}))
        self.write(PLAN_DIR + 'finish-log.tsv', FINISH_HEAD)
        git(self.repo, 'add', '-A')
        git(self.repo, 'commit', '-q', '-m', 'finish tools')

    def targets(self, *cases):
        """[key, package id, batch ids, warning] of the real gaps.target(root, arg) for each (root, arg), in a fresh process."""
        r = subprocess.run([sys.executable, '-c', TARGET_DRIVER, str(self.repo / '.claude/skills/finish/gaps.py'),
                            json.dumps([[str(root), arg] for root, arg in cases])], capture_output=True, text=True, env=self.env, timeout=60)
        self.assertEqual((r.returncode, r.stderr), (0, ''))
        return [json.loads(l) for l in r.stdout.splitlines()]

    def test_gaps_target(self):
        """The real gaps.target(): a batch id wins over a package id, arg before the branch and worktree names (a warning when arg
        names neither), else the branch itself as the key; a claim branch Pxxx-<batch>-<user> takes its package's first batch
        the finish log has not landed."""
        self.real_finish()
        r, warn = self.repo, 'no package or batch id in "{}": the branch and worktree names decide'.format
        self.assertEqual(self.targets((r, ''), (r, 'SUP32'), (r, 'p018'), (r, 'P900'), (r, 'P900 SUP33'), (r, '"SUP33"'), (r, 'foo')),
                         [['master', None, [], ''], ['SUP32', 'P900', ['SUP32'], ''], ['P018', 'P018', ['C18'], ''],
                          ['P900', 'P900', ['SUP32', 'SUP33'], ''], ['SUP33', 'P900', ['SUP33'], ''], ['SUP33', 'P900', ['SUP33'], ''],
                          ['master', None, [], warn('foo')]])
        git(r, 'checkout', '-q', '-b', 'feat/P018-x')
        git(r, 'branch', 'SUP32-P018')
        self.assertEqual(self.targets((r, ''), (r, 'bar')), [['P018', 'P018', ['C18'], ''], ['P018', 'P018', ['C18'], warn('bar')]])
        git(r, 'checkout', '-q', 'SUP32-P018')
        self.assertEqual(self.targets((r, '')), [['SUP32', 'P900', ['SUP32'], '']], 'a batch id wins in the branch name')
        git(r, 'checkout', '-q', '-b', 'feat/x')
        self.assertEqual(self.targets((r, '')), [['feat_x', None, [], '']])
        git(r, 'checkout', '-q', '--detach')
        self.assertEqual(self.targets((r, '')), [['detached', None, [], '']])
        git(r, 'checkout', '-q', 'master')
        wt, claim = self.tmp / 'SUP33', self.tmp / 'claim'
        git(r, 'worktree', 'add', '-q', '-b', 'work', str(wt))
        git(r, 'worktree', 'add', '-q', '-b', 'P900-SUP32-alice', str(claim))
        self.assertEqual(self.targets((wt, ''), (wt, 'P018'), (claim, ''), (claim, 'SUP32')),
                         [['SUP33', 'P900', ['SUP33'], ''], ['P018', 'P018', ['C18'], ''], ['SUP32', 'P900', ['SUP32'], ''],
                          ['SUP32', 'P900', ['SUP32'], '']], 'the worktree folder name; a claim branch with nothing landed')
        (claim / PLAN_DIR / 'finish-log.tsv').write_text(FINISH_HEAD + '2026-11-24\tSUP32\tm\thigh\t0\t0\t0\tverified\n'
                                                         '2026-11-24\tSUP33\tm\thigh\t0\t0\t0\tpartial\n')
        self.assertEqual(self.targets((claim, ''), (claim, 'SUP32'), (r, '')),
                         [['SUP33', 'P900', ['SUP33'], ''], ['SUP32', 'P900', ['SUP32'], ''], ['master', None, [], '']],
                         'a claim branch skips its landed first batch (only a verified line lands one); a typed id is kept')
        with open(claim / PLAN_DIR / 'finish-log.tsv', 'a') as f:
            f.write('2026-11-24\tSUP33\tm\thigh\t0\t0\t0\tverified\n')
        self.assertEqual(self.targets((claim, '')), [['SUP32', 'P900', ['SUP32'], '']], 'every batch landed: the branch name decides')

    def test_pause_bound(self):
        """pause.py's facts fit any re-attachment room unfitted (SUP34): 24 hours, its most, print at most plan.FLOOR (1,500)
        characters, the least room a script fits to, and more hours are refused."""
        self.real_finish()
        run = lambda hours: subprocess.run([sys.executable, str(self.repo / '.claude/skills/pause/pause.py'), hours, 's1'], cwd=self.repo,
                                           capture_output=True, text=True, env=self.env | {'HOME': str(self.tmp / 'home')}, timeout=60)
        r = run('24')
        self.assertEqual((r.returncode, r.stderr), (0, ''))
        self.assertEqual(r.stdout.count('\n- `'), 28, r.stdout)
        self.assertLessEqual(len(r.stdout), 1500, r.stdout)
        r = run('24.01')
        self.assertEqual((r.returncode, r.stderr), (1, '/pause takes 0 to 24 hours (a session holds at most 50 scheduled jobs)\n'))

    def test_pause_key(self):
        """pause.py's key: the real gaps.target() of the real resume.last_command() over the session's transcript, found under
        ~/.claude/projects/*/<session>.jsonl: the newest typed /package, /prune or /finish with an argument (/finish without one
        keeps the earlier), never a quoted command, another command or a broken line; else the branch or worktree name."""
        self.real_finish()
        quote = '<command-name>/prune</command-name>\n<command-args>P018</command-args>'
        quoted = [{'type': 'user', 'message': {'role': 'user', 'content': [{'type': 'tool_result', 'tool_use_id': 't', 'content': quote}]}},
                  {'type': 'user', 'message': {'role': 'user', 'content': [{'type': 'text', 'text': quote}]}},
                  {'type': 'assistant', 'message': {'content': quote}}, {'type': 'system', 'content': quote}]
        broken = b'{"type": "user", "message": {"content": "<command-name>/prune</command-name><command-args>P018</command-args>"\n'

        def key(cwd, *recs, extra=b'', session='s1'):
            proj = self.tmp / 'home/.claude/projects/-x'
            proj.mkdir(parents=True, exist_ok=True)
            (proj / 's1.jsonl').write_bytes(''.join(json.dumps(x) + '\n' for x in recs).encode() + extra)
            r = subprocess.run([sys.executable, str(cwd / '.claude/skills/pause/pause.py'), '0', session], cwd=cwd, capture_output=True,
                               text=True, env=self.env | {'HOME': str(self.tmp / 'home')}, timeout=60)
            self.assertEqual((r.returncode, r.stderr), (0, ''), recs)
            m = re.search(r'\bkey (\S+)', r.stdout)
            self.assertTrue(m, r.stdout)
            return m[1]
        r = self.repo
        cases = (([typed('package', 'SUP32')], b'', 'SUP32'), ([typed('prune', 'P018')], b'', 'P018'), ([typed('finish', 'SUP33')], b'', 'SUP33'),
                 ([typed('package', 'SUP32'), typed('finish')], b'', 'SUP32'), ([typed('finish', 'SUP33'), typed('prune', ' P018  extra')], b'', 'P018'),
                 ([typed('package', '"P018"')], b'', 'P018'), ([typed('package', "'SUP33' --x")], b'', 'SUP33'),
                 ([typed('package', 'SUP32'), *quoted], broken, 'SUP32'), ([*quoted], broken, 'master'),
                 ([typed('pause', 'SUP32'), typed('packages', 'P018'), typed('prune-all', 'P018')], b'', 'master'),
                 (['<command-name>/package</command-name>', {'type': 'user', 'message': '<command-name>/prune</command-name>'},
                   typed('finish', 'C18')], b'\xff\xfe garbage\n', 'C18'),
                 ([typed('package', 'foo')], b'', 'master'), ([], b'', 'master'))
        for recs, extra, want in cases:
            self.assertEqual(key(r, *recs, extra=extra), want, recs)
        self.assertEqual(key(r, typed('package', 'SUP32'), session='other'), 'master', 'no transcript for the session')
        wt = self.tmp / 'wt'
        git(r, 'worktree', 'add', '-q', '-b', 'P900-SUP32-bob', str(wt))
        self.assertEqual([key(wt), key(wt, typed('prune', 'P018')), key(r, typed('finish'))], ['SUP32', 'P018', 'master'],
                         'a linked worktree on a claim branch: its name without a typed command, the typed one with it')

    def notify(self, payload=None, raw=None, cwd=None, path=None, via=None, pre=(), **env):
        """(exit code, stdout and stderr, [argv of each notify-send call]) of notify.py, the fake notify-send first on PATH.
        It runs as the child of a shell (via: another copy of one, as a fake editor; pre: that shell's own arguments) whose
        environment lacks TERM_PROGRAM, so that notify.py's ancestor walk stops there instead of reaching the editor whose
        terminal may run these tests."""
        log = self.tmp / 'notify.log'
        log.unlink(missing_ok=True)
        e = {k: v for k, v in self.env.items() if k not in ('TERM_PROGRAM', 'WSL_DISTRO_NAME') and not k.startswith('VSCODE_')} | {
            'PATH': path or f'{self.tmp / "bin"}{os.pathsep}{Path(shutil.which("git")).parent}', 'NOTIFY_LOG': str(log),
            'XDG_CONFIG_HOME': str(self.tmp / 'config')} | env
        term = {'VIA_TERM': e.pop('TERM_PROGRAM')} if 'TERM_PROGRAM' in e else {}
        r = subprocess.run([via or SH, '-c', '[ -z "$VIA_PID" ] || echo $$ > "$VIA_PID"; [ -z "$VIA_TERM" ] || export TERM_PROGRAM="$VIA_TERM"; '
                            'shift "$1"; "$@"; exit $?', 'sh', str(len(pre) + 1), *pre, sys.executable, str(self.repo / '.claude/hooks/notify.py')], input=(raw if raw is not None else json.dumps(payload)).encode()
                           if not isinstance(raw, bytes) else raw, capture_output=True, cwd=cwd or self.tmp / 'plain dir', env=e | term, timeout=60)
        rc, out, err = r.returncode, r.stdout.decode(), r.stderr.decode()
        calls = [c.split('\0')[:-1] for c in log.read_text().split('\x1e')[:-1]] if log.exists() else []
        return rc, out + err, calls

    def test_notify(self):
        self.write('.claude/hooks/notify.py', (HOOKS / 'notify.py').read_text())
        fake = self.tmp / 'bin/notify-send'
        fake.parent.mkdir()
        fake.write_text(FAKE_NOTIFY)
        fake.chmod(0o755)
        for d in ('plain dir', 'empty'):
            (self.tmp / d).mkdir()
        app, base = '--app-name=Claude Code', {'session_id': 's1', 'hook_event_name': 'Notification', 'cwd': str(self.repo)}

        def title(name, compact=False):
            return json.dumps({'type': 'custom-title', 'customTitle': name, 'sessionId': 's1'}, separators=(',', ':') if compact else None) + '\n'

        def sent(header, body, **kw):
            self.assertEqual(self.notify(base | kw), (0, '', [[app, f'Claude Code · {header}', body]]), (header, body, kw))
        t = self.tmp / 'session.jsonl'
        quote = {'type': 'user', 'message': {'role': 'user', 'content': [{'type': 'tool_result', 'tool_use_id': 'toolu_1',
                                                                          'content': '{"type": "custom-title", "customTitle": "quoted"}'}]}}
        t.write_text(title('Old name') + json.dumps({'type': 'assistant', 'message': {'content': 'x'}}) + '\n' + title('SUP32 tests', True)
                     + json.dumps(quote) + '\n')
        sent('SUP32 tests', 'waits for your permission: Claude needs your permission to use Bash', transcript_path=str(t),
             notification_type='permission_prompt', message='Claude needs your permission to use Bash')
        for message in (None, '', 5, ['x']):
            sent('SUP32 tests', 'waits for your input', transcript_path=str(t), notification_type='idle_prompt',
                 **({} if message is None else {'message': message}))
        sent('SUP32 tests', 'needs attention: Signed in', transcript_path=str(t), notification_type='auth_success', message='Signed in')
        sent('SUP32 tests', 'needs attention', transcript_path=str(t))
        sent('SUP32 tests', 'needs attention', transcript_path=str(t), notification_type=7)
        t.write_text(title('  Fix "quotes" – ü\\path  '))
        sent('Fix "quotes" – ü\\path', 'waits for your input', transcript_path=str(t), notification_type='idle_prompt')
        t.write_text(title('T' * 200))
        rc, out, calls = self.notify(base | {'transcript_path': str(t), 'notification_type': 'idle_prompt', 'message': 'm' * 400})
        self.assertEqual((rc, out, calls), (0, '', [[app, ('Claude Code · ' + 'T' * 200)[:120], ('waits for your input: ' + 'm' * 400)[:300]]]))
        filler = json.dumps({'type': 'assistant', 'message': {'content': 'y' * (1 << 20)}}) + '\n'
        t.write_text(title('Early') + filler + filler)
        sent('Early', 'waits for your input', transcript_path=str(t), notification_type='idle_prompt')
        t.write_text(title('Lost') + filler + title('Found'))
        sent('Found', 'waits for your input', transcript_path=str(t), notification_type='idle_prompt')
        for tp in ({}, {'transcript_path': str(self.tmp / 'missing.jsonl')}, {'transcript_path': str(self.transcript(quote))}, {'transcript_path': 5.5}):
            sent('repo (master)', 'waits for your input', notification_type='idle_prompt', **tp)
        sent('repo (master)', 'waits for your input', notification_type='idle_prompt', cwd=str(self.repo / '05-M'))
        git(self.repo, 'checkout', '-q', '-b', 'P900-SUP32-x')
        sent('repo (P900-SUP32-x)', 'waits for your input', notification_type='idle_prompt')
        git(self.repo, 'checkout', '-q', '--detach')
        sent('repo', 'waits for your input', notification_type='idle_prompt')
        git(self.repo, 'checkout', '-q', 'master')
        sent('plain dir', 'waits for your input', notification_type='idle_prompt', cwd=str(self.tmp / 'plain dir'))
        self.assertEqual(self.notify(raw='{}', cwd=self.tmp / 'plain dir'), (0, '', [[app, 'Claude Code · plain dir', 'needs attention']]),
                         'no cwd outside git: the working directory\'s name, never empty')
        sent('repo (master)', 'needs attention', notification_type=['idle_prompt'])
        for cwd in (str(self.tmp / 'gone'), 7, None):  # the hook's working directory, as Claude Code starts it in the project
            self.assertEqual(self.notify(base | {'notification_type': 'idle_prompt', 'cwd': cwd}, cwd=self.repo),
                             (0, '', [[app, 'Claude Code · repo (master)', 'waits for your input']]), cwd)
        for raw in ('not json', '', b'\xff\xfe\x00garbage', '[]', 'null', '5', '{"cwd": '):
            self.assertEqual(self.notify(raw=raw, cwd=self.repo), (0, '', [[app, 'Claude Code · repo (master)', 'needs attention']]), raw)
        payload = base | {'notification_type': 'idle_prompt'}
        self.assertEqual(self.notify(payload, path=str(self.tmp / 'empty')), (0, '', []), 'no notify-send on PATH')
        self.assertFalse((self.tmp / 'notify.log').exists())
        self.assertEqual(self.notify(payload, NOTIFY_EXIT='3'), (0, '', [[app, 'Claude Code · repo (master)', 'waits for your input']]),
                         'notify-send failing')
        start = time.monotonic()
        self.assertEqual(self.notify(payload, NOTIFY_SLEEP='30'), (0, '', [[app, 'Claude Code · repo (master)', 'waits for your input']]),
                         'notify-send hanging')
        self.assertLess(time.monotonic() - start, 15, 'a hanging notify-send is cut off')

    def fakes(self):
        """The fake notify-send and code in tmp/bin beside links to TOOLS: a PATH without the real code."""
        (self.tmp / 'bin').mkdir(exist_ok=True)
        for name, text in (('notify-send', FAKE_NOTIFY), ('code', FAKE_CODE)):
            (self.tmp / 'bin' / name).write_text(text)
            (self.tmp / 'bin' / name).chmod(0o755)
        for tool in TOOLS:
            if not (self.tmp / 'bin' / tool).exists():
                (self.tmp / 'bin' / tool).symlink_to(shutil.which(tool))
        return str(self.tmp / 'bin')

    @staticmethod
    def wait(path, seconds=20):
        end = time.monotonic() + seconds
        while not path.exists() and time.monotonic() < end:
            time.sleep(0.05)
        return path.exists()

    def test_notify_click(self):
        """Linux in VS Code's terminal: the hook returns at once, a detached helper in its own session notifies with a
        default action and, on a click, runs `code <window folder>` without the session's variables and inherited
        activation tokens; on KDE KWin first activates the window whose title names that folder. The editor is found from
        the hook's ancestors (a fake editor: a copy of the shell beside a product.json) before the askpass script."""
        self.write('.claude/hooks/notify.py', (HOOKS / 'notify.py').read_text())
        path = self.fakes()
        log, code_log, gate, sid = self.tmp / 'notify.log', self.tmp / 'code.log', self.tmp / 'gate', self.tmp / 'sid'
        spaced = self.repo / 'My Repo'
        for d in (self.repo / '05-M', spaced / 'sub', self.tmp / 'plain dir', self.tmp / 'repo2'):
            d.mkdir(parents=True, exist_ok=True)
        payload = {'notification_type': 'idle_prompt', 'cwd': str(self.repo / '05-M')}
        kept = ['VSCODE_IPC_HOOK_CLI']

        def uri(d):
            return 'file://' + quote(str(d))

        def clicked(want, cwd=None, windows=None, storage=None, header='repo (master)', name='Code', bins=None, url=..., **env):
            """url: the argv of the `code --open-url` call after code, None for none, ... unchecked."""
            for f in (code_log, gate, sid, Path(f'{code_log}.url')):
                f.unlink(missing_ok=True)
            config = self.tmp / 'config'
            shutil.rmtree(config, ignore_errors=True)
            if windows is not None:
                storage = json.dumps({'windowsState': {'lastActiveWindow': {'folder': uri(windows[0])},
                                                       'openedWindows': [{'folder': uri(d)} for d in windows[1:]]}})
            if storage is not None:
                (config / name / 'User/globalStorage').mkdir(parents=True)
                (config / name / 'User/globalStorage/storage.json').write_text(storage)
            env = SESSION_VARS | {'TERM_PROGRAM': 'vscode', 'CODE_LOG': str(code_log), 'NOTIFY_GATE': str(gate),
                                  'NOTIFY_SID': str(sid), 'XDG_ACTIVATION_TOKEN': 'stale', 'DESKTOP_STARTUP_ID': 'stale'} | env
            self.assertEqual(self.notify(payload | ({'cwd': cwd} if cwd else {}), path=bins or path, **env), (0, '', []),
                             'the hook returns before the helper notifies')
            gate.touch()
            self.assertTrue(self.wait(sid), 'the helper notifies')
            helper, keep = sid.read_text().split()
            self.assertNotEqual(helper, str(os.getsid(0)), 'the helper runs in its own session')
            self.assertEqual(keep, 'none', 'no CP_REAP_KEEP, even inherited: /reap --own and SessionEnd end the helper')
            argv = log.read_text().split('\x1e')[0].split('\0')[:-1]
            self.assertEqual(argv, ['--app-name=Claude Code', '--action=default=Open', f'Claude Code · {header}', 'waits for your input'])
            if want is None:
                time.sleep(1)
                self.assertFalse(code_log.exists(), 'no click: VS Code is left alone')
            else:
                self.assertTrue(self.wait(code_log), 'a click runs code')
                self.assertEqual(code_log.read_text().split('\0')[:-1], want)
            if url is None:
                time.sleep(0.5)
                self.assertFalse(Path(f'{code_log}.url').exists(), 'no terminal-focus URI')
            elif url is not ...:
                self.assertTrue(self.wait(Path(f'{code_log}.url')), 'the terminal-focus URI opened after code')
                self.assertRegex('\0'.join(Path(f'{code_log}.url').read_text().split('\0')[:-1]), url)
        tok = ['token=', 'startup='] + kept  # inherited activation tokens never reach code
        clicked([str(self.repo)] + tok)
        kde, kwin = self.tmp / 'kde', self.tmp / 'kwin.log'  # KDE: KWin activates the window before code runs
        kde.mkdir()
        for f in (self.tmp / 'bin').iterdir():
            (kde / f.name).symlink_to(f)
        (kde / 'gdbus').write_text(FAKE_DBUS)
        (kde / 'gdbus').chmod(0o755)
        clicked([str(self.repo)] + tok, bins=str(kde), XDG_CURRENT_DESKTOP='KDE', KWIN_LOG=str(kwin), KWIN_OUT='(3,)')
        calls = [line.split('\0')[:-1] for line in kwin.read_text().split('\n') if line]
        self.assertEqual([c[7] for c in calls], ['org.kde.kwin.Scripting.loadScript', 'org.kde.kwin.Script.run', 'org.kde.kwin.Scripting.unloadScript'])
        self.assertEqual(calls[1][5], '/Scripting/Script3')
        self.assertIn(f'const path = " - " + {json.dumps(str(self.repo))} + " - ", name = " - " + "repo" + " - ", app = "Visual Studio Code", '
                      'pid = 0, mark = "";', Path(f'{kwin}.js').read_text())
        self.assertFalse(Path(f'{kwin}.order').exists(), 'KWin activates before code runs')
        clicked([str(self.repo)] + tok, NOTIFY_HELP='--urgency --action --wait')
        clicked(None, NOTIFY_ACTION='')
        clicked([str(self.repo / '05-M')] + tok, windows=[self.tmp, self.repo / '05-M'])
        clicked([str(self.repo / '05-M')] + tok, windows=[self.repo / '05-M', self.tmp], cwd=str(self.repo / '05-M/'))
        clicked([str(self.tmp)] + tok, windows=[self.tmp], cwd=str(self.repo))
        clicked([str(spaced)] + tok, windows=[self.tmp, spaced], cwd=str(spaced / 'sub'))
        clicked([str(self.tmp / 'repo2')] + tok, windows=[self.repo], cwd=str(self.tmp / 'repo2'), header='repo2')
        for bad in ('not json', '[]', '{"windowsState": 5}', '{"windowsState": {"openedWindows": 5}}',
                    json.dumps({'windowsState': {'openedWindows': [{'folder': 'vscode-remote://x/y'}, {}, 5]}})):
            clicked([str(self.repo)] + tok, storage=bad)
        wt = self.tmp / 'wt'
        git(self.repo, 'worktree', 'add', '-q', '-b', 'P900-SUP32-wt', str(wt))
        clicked([str(self.repo)] + tok, cwd=str(wt), header='wt (P900-SUP32-wt)')
        app = self.tmp / 'cursor app'  # an editor built from VS Code, found from its git extension's askpass script
        (app / 'resources/app/extensions/git/dist').mkdir(parents=True)
        (app / 'resources/app/product.json').write_text(json.dumps({'nameShort': 'Cursor', 'applicationName': 'cursor', 'urlProtocol': 'cursor'}))
        (app / 'bin').mkdir()
        shutil.copy(self.tmp / 'bin/code', app / 'bin/cursor')
        askpass = {'VSCODE_GIT_ASKPASS_MAIN': str(app / 'resources/app/extensions/git/dist/askpass-main.js')}
        no_code = self.tmp / 'nocode'
        no_code.mkdir()
        for f in ('notify-send', *TOOLS):
            (no_code / f).symlink_to(self.tmp / 'bin' / f)
        clicked([str(self.repo / '05-M')] + tok, windows=[self.repo / '05-M'], name='Cursor', bins=str(no_code), **askpass)
        clicked([str(self.repo)] + tok, windows=[self.repo / '05-M'], name='Code', bins=str(no_code), **askpass)
        (app / 'bin/cursor').unlink()
        (app / 'resources/app/bin').mkdir()
        shutil.copy(self.tmp / 'bin/code', app / 'resources/app/bin/cursor')
        clicked([str(self.repo)] + tok, bins=str(no_code), **askpass)
        # git.enabled off (no askpass): the editor found from the hook's ancestors, a fake editor whose --user-data-dir holds
        # the window list, its own launcher and pid (KWin), and the terminal-focus extension's URI after code when its folder
        # sits in the extensions folder; both launches carry its --user-data-dir and --extensions-dir; it wins over the
        # askpass script; a key its product.json lacks never falls back
        ed, udd, home, ed_pid = self.tmp / 'ed app', self.tmp / 'ed data', self.tmp / 'home', self.tmp / 'ed.pid'
        (ed / 'resources/app').mkdir(parents=True)
        ed_product = {'nameShort': 'Ed', 'nameLong': 'Ed Long', 'applicationName': 'ed-cli', 'urlProtocol': 'ed', 'dataFolderName': '.ed'}
        (ed / 'resources/app/product.json').write_text(json.dumps(ed_product))
        (ed / 'bin').mkdir()
        shutil.copy(self.tmp / 'bin/code', ed / 'bin/ed-cli')
        shutil.copy(SH, ed / 'ed')
        (udd / 'User/globalStorage').mkdir(parents=True)
        (udd / 'User/globalStorage/storage.json').write_text(json.dumps({'windowsState': {'openedWindows': [{'folder': uri(self.repo / '05-M')}]}}))
        exts = home / '.ed/extensions'
        via = {'via': str(ed / 'ed'), 'pre': (f'--user-data-dir={udd}', '--extensions-dir', str(exts)), 'VIA_PID': str(ed_pid), 'HOME': str(home),
               'bins': str(no_code)}
        focus = r'ed://compprog\.terminal-focus/focus\?pid=([0-9]+)'
        own = ['--user-data-dir', str(udd), '--extensions-dir', str(exts)]
        clicked(own + [str(self.repo / '05-M')] + tok, url=None, **via)
        ext = exts / 'compprog.terminal-focus-0.0.1'
        ext.mkdir(parents=True)
        clicked(own + [str(self.repo / '05-M')] + tok, url=None, **via)  # a folder without package.json
        (ext / 'package.json').write_text('{}')
        clicked(own + [str(self.repo / '05-M')] + tok, url=re.escape('\0'.join(own)) + '\0--open-url\0' + focus, **via, **askpass)
        shell = int(re.search(focus, Path(f'{code_log}.url').read_text())[1])
        self.assertTrue(shell > 1 and shell != int(ed_pid.read_text()), 'the pid of the process below the editor')
        no_kde = self.tmp / 'nocode-kde'
        no_kde.mkdir()
        for f in (self.tmp / 'bin').iterdir():
            if f.name != 'code':
                (no_kde / f.name).symlink_to(f)
        (no_kde / 'gdbus').symlink_to(kde / 'gdbus')
        clicked(own + [str(self.repo / '05-M')] + tok, **via | {'bins': str(no_kde)}, XDG_CURRENT_DESKTOP='KDE', KWIN_LOG=str(kwin), KWIN_OUT='(4,)')
        self.assertIn(f', app = "Ed Long", pid = {int(ed_pid.read_text())}, mark = "";', Path(f'{kwin}.js').read_text(),
                      "KWin gets the editor's main process")
        (ed / 'resources/app/product.json').write_text(json.dumps({k: v for k, v in ed_product.items() if k != 'applicationName'}))
        self.assertEqual(self.notify(payload, CODE_LOG=str(code_log), TERM_PROGRAM='vscode', **via | {'path': path}, **askpass),
                         (0, '', [['--app-name=Claude Code', 'Claude Code · repo (master)', 'waits for your input']]),
                         "no applicationName: no launcher, neither the askpass editor's nor code on PATH")
        plain = [['--app-name=Claude Code', 'Claude Code · repo (master)', 'waits for your input']]
        for env, why in (({'path': path}, 'outside VS Code'), ({'TERM_PROGRAM': 'vscode', 'path': str(no_code)}, 'no code on PATH'),
                         ({'TERM_PROGRAM': 'vscode', 'path': path, 'NOTIFY_HELP': '--urgency'}, 'notify-send without --action')):
            self.assertEqual(self.notify(payload, CODE_LOG=str(code_log), **env), (0, '', plain), why)

    def imported(self):
        """A fresh import of notify.py: its deadline an hour ahead, host() finding nothing (no /proc)."""
        spec = importlib.util.spec_from_file_location('notify_copy', HOOKS / 'notify.py')
        n = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(n)
        n.DEADLINE, n.PROC = time.monotonic() + 3600, self.tmp / 'no proc'
        return n

    def test_notify_host(self):
        """host() over a fake /proc: up through ancestors started with TERM_PROGRAM=vscode to the first whose executable has
        a product.json (resources/app beside it, or beside it for a server), the shell below it, the topmost ancestor with
        the same executable, --user-data-dir and --extensions-dir from either command line form (NUL-separated, or one
        rewritten title string) nearest first; {} for an outside ancestor first, a missing entry, a loop, pid 1, no /proc.
        editor() takes the found product before the askpass one and never falls back to it; extensions() and focuser()."""
        n = self.imported()
        proc, app, server = self.tmp / 'proc', self.tmp / 'Ed App', self.tmp / 'server'
        (app / 'resources/app').mkdir(parents=True)
        (app / 'resources/app/product.json').write_text(json.dumps({'nameShort': 'Ed', 'urlProtocol': 'ed', 'dataFolderName': '.ed'}))
        server.mkdir()
        (server / 'product.json').write_text('{}')
        term, code = b'A=1\0TERM_PROGRAM=vscode\0', str(app / 'code')

        def ps(pid, ppid, exe, cmdline=b'x\0', environ=term, comm='x'):
            d = proc / str(pid)
            shutil.rmtree(d, ignore_errors=True)
            d.mkdir(parents=True)
            (d / 'stat').write_bytes(f'{pid} ({comm}) S {ppid} 1 1 0'.encode())
            if exe:
                (d / 'exe').symlink_to(exe)
            (d / 'cmdline').write_bytes(cmdline)
            (d / 'environ').write_bytes(environ)

        def chain(over=None):
            shutil.rmtree(proc, ignore_errors=True)
            rows = {100: (90, '/usr/bin/python3'), 90: (70, '/bin/sh', b'sh\0-c\0x\0', term, 'a) S 77 (b'), 70: (60, '/bin/bash'),
                    60: (50, code, f'{code} --type=utility --user-data-dir=/u d/x --lang=en'.encode(), b''),
                    50: (40, code, f'{code}\0--extensions-dir\0/e x\0--user-data-dir=/other\0'.encode(), b''), 40: (1, '/lib/systemd/systemd')}
            for pid, row in (rows | (over or {})).items():
                if row:
                    ps(pid, *row)
            n.PROC = proc
            return n.host(100)
        want = {'app': app / 'resources/app', 'shell': 70, 'pid': 50, 'data': '/u d/x', 'extensions': '/e x'}
        self.assertEqual(chain(), want)
        self.assertEqual(chain({45: (40, code, b'code\0--user-data-dir\0/main\0', b''), 50: (45, code, b'code\0', b'')}),
                         want | {'pid': 45, 'extensions': None}, 'up to the topmost same executable; the nearest value')
        self.assertEqual(chain({60: (50, code, f'{code} --user-data-dir=/d'.encode(), b''), 50: (40, '/other/code')}),
                         want | {'pid': 60, 'data': '/d', 'extensions': None}, 'a value at the end of a title; another executable above')
        self.assertEqual(chain({50: (40, None)}), want | {'pid': 60, 'extensions': None}, 'an unreadable executable above')
        self.assertEqual(chain({70: (60, '/bin/bash', b'x\0', b'TERM_PROGRAM=vscodex\0')}), {}, 'an ancestor outside the terminal first')
        self.assertEqual(chain({90: (70, '/bin/sh', b'x\0', b'A=1\0')}), {}, 'an ancestor started without TERM_PROGRAM')
        self.assertEqual(chain({70: None}), {}, 'a missing ancestor')
        self.assertEqual(chain({70: (90, '/bin/bash')}), {}, 'a loop ends after HOPS')
        self.assertEqual(chain({70: (1, '/bin/bash')}), {}, 'pid 1 reached')
        self.assertEqual(chain({60: (50, str(server / 'node'), b'node\0', b'')}),
                         {'app': server, 'shell': 70, 'pid': 60, 'data': None, 'extensions': None}, "a server's node beside product.json")
        chain()
        self.assertEqual(n.host(60), want | {'shell': None}, 'started at the editor')
        n.PROC = self.tmp / 'no proc'
        self.assertEqual(n.host(100), {}, 'no /proc')
        for line, want_value in ((b'a\0--user-data-dir\0/x y\0', '/x y'), (b'a\0--user-data-dir\0', None), (b'--user-data-dir=/a --b', '/a'),
                                 (b'a --user-data-dirx=/a', None), (b'a\0--user-data-dirx=/a\0', None), (b'a', None)):
            self.assertEqual(n.option(line, 'user-data-dir'), want_value, line)
        found = {'app': app / 'resources/app', 'shell': 70, 'pid': 50, 'data': None, 'extensions': None}
        cursor = self.tmp / 'cursor/resources/app'
        (cursor / 'extensions/git/dist').mkdir(parents=True)
        (cursor / 'product.json').write_text(json.dumps({'nameShort': 'Cursor', 'applicationName': 'cursor'}))
        with unittest.mock.patch.dict(os.environ, {'VSCODE_GIT_ASKPASS_MAIN': str(cursor / 'extensions/git/dist/askpass-main.js')}), \
                unittest.mock.patch.object(n, 'host', lambda: found):
            self.assertEqual(n.editor(), ({'nameShort': 'Ed', 'urlProtocol': 'ed', 'dataFolderName': '.ed'}, app / 'resources/app', found))
            (app / 'resources/app/product.json').write_text('not json')
            self.assertEqual(n.editor(), (n.VSCODE, None, found), "a broken product.json never falls back to the askpass editor's")
        ed = {'nameShort': 'Ed', 'urlProtocol': 'ed', 'dataFolderName': '.ed'}
        home, ext = self.tmp / 'home', self.tmp / 'home/.ed/extensions'
        with unittest.mock.patch.object(n.Path, 'home', staticmethod(lambda: home)), unittest.mock.patch.dict(os.environ, {}):
            os.environ.pop('VSCODE_PORTABLE', None)
            self.assertEqual(n.extensions(ed, found | {'extensions': '/e'}), Path('/e'))
            self.assertEqual(n.extensions(ed, found), ext)
            self.assertIsNone(n.extensions({}, found))
            os.environ['VSCODE_PORTABLE'] = '/p'
            self.assertEqual(n.extensions({}, found), Path('/p/extensions'))
            os.environ.pop('VSCODE_PORTABLE')
            self.assertIsNone(n.focuser(ed, found), 'no extensions folder')
            for name, pkg, uri in (('compprog.terminal-focusx', True, None), ('compprog.terminal-focus-0.0.1', False, None),
                                   ('compprog.terminal-focus-0.0.1', True, 'ed://compprog.terminal-focus/focus?pid=70'),
                                   ('compprog.terminal-focus', True, 'ed://compprog.terminal-focus/focus?pid=70')):
                shutil.rmtree(ext, ignore_errors=True)
                (ext / name).mkdir(parents=True)
                if pkg:
                    (ext / name / 'package.json').write_text('{}')
                self.assertEqual(n.focuser(ed, found), uri, name)
            self.assertEqual(n.focuser(ed, found | {'shell': None}), f'ed://compprog.terminal-focus/focus?pid={os.getppid()}', "the hook's parent")
            self.assertIsNone(n.focuser({'dataFolderName': '.ed'}, found), 'no urlProtocol')

    def kwin_run(self, js, windows, client=False):
        """{active window id, minimized flags, printed lines} of a KWin script run by Node.js against fake windows."""
        harness = ('const fs = require("fs"); const [file, wins, client] = process.argv.slice(1); const out = [];'
                   'globalThis.print = (...a) => out.push(a.join(" ")); const list = JSON.parse(wins);'
                   'globalThis.workspace = client === "1" ? {clientList: () => list, activeClient: null} : {windowList: () => list, activeWindow: null};'
                   'eval(fs.readFileSync(file, "utf8")); const a = workspace.activeWindow || workspace.activeClient;'
                   'console.log(JSON.stringify([a ? a.id : null, list.map(w => w.minimized), out]));')
        f = self.tmp / 'kwin.js'
        f.write_text(js)
        r = subprocess.run([NODE, '-e', harness, str(f), json.dumps(windows), '1' if client else '0'], capture_output=True, text=True, timeout=60)
        self.assertEqual((r.returncode, r.stderr), (0, ''), js)
        return json.loads(r.stdout)

    @unittest.skipUnless(NODE, 'Node.js missing')
    def test_notify_kwin_script(self):
        """script() run by Node.js on fake KWin windows: the editor's pid (4) beats the title holding the folder's path as
        the project's window.title renders it, ~/ under home (2), which beats its name (1); title matches need the title to
        end with the app; only normal windows; the first of a tie; the winner unminimized and activated (activeClient on old
        KWin); the probe mode prints the counts and the winner's title and changes nothing."""
        n = self.imported()
        home, app = self.tmp / 'home', 'Visual Studio Code'
        folder, under = '/srv/My "Repo"', str(home / 'w x')
        title = DEFAULT_TITLE.replace('${rootName}', '${rootPath}')

        def caption(path, editor='f.md', dirty=''):  # the window.title rendered as VS Code joins non-empty parts
            parts = {'${dirty}': dirty, '${activeEditorShort}': editor, '${rootPath}': path, '${profileName}': '', '${appName}': app}
            return dirty + ' - '.join(p for p in (parts[k] for k in ('${activeEditorShort}', '${rootPath}', '${profileName}', '${appName}')) if p)

        def win(i, cap, pid=1, normal=True):
            return {'id': i, 'caption': cap, 'pid': pid, 'normalWindow': normal, 'minimized': True}
        self.assertEqual(caption(folder, dirty='● '), f'● f.md - {folder} - {app}')
        with unittest.mock.patch.object(n.Path, 'home', staticmethod(lambda: home)):
            js = n.script(folder, app, 77)
            for wins, active, why in (
                    ([win(1, caption('/other')), win(2, caption(folder), 5), win(3, caption('/x'), 77)], 3, 'pid beats the title'),
                    ([win(1, f'f.md - My "Repo" - {app}', 77), win(2, caption(folder), 77)], 2, 'pid and path beat pid and name'),
                    ([win(1, f'My "Repo" - {app}', 5), win(2, caption(folder, dirty='● '), 5)], 2, 'path beats name'),
                    ([win(1, f'f.md - My "Repo" - {app}', 5)], 1, 'the name alone'),
                    ([win(1, f'f.md - {folder} - Konsole', 5), win(2, f'f.md - My "Repo" - {app} x', 5)], None, 'the title must end with the app'),
                    ([win(1, caption(folder), 77, normal=False)], None, 'normal windows only'),
                    ([win(1, caption('/a'), 77), win(2, caption('/b'), 77)], 1, 'the first of a tie'),
                    ([win(1, caption(folder + 'x'), 5), win(2, caption('/srv'), 5)], None, 'another folder')):
                got = self.kwin_run(js, wins)
                self.assertEqual(got, [active, [w['id'] != active for w in wins], []], why)
            self.assertEqual(self.kwin_run(n.script(under, app, 0), [win(1, caption(under)), win(2, caption('~/w x'))]), [2, [True, False], []],
                             'under home: ~/ as VS Code tildifies it')
            self.assertEqual(self.kwin_run(n.script(folder, '', 0), [win(1, f'f.md - {folder} - ')]), [None, [True], []], 'no app: no title match')
            self.assertEqual(self.kwin_run(js, [win(1, caption('/x'), 77)], client=True), [1, [False], []], 'activeClient on KWin 5')
            probe = n.script(folder, app, 77, 'mark-1')
            wins = [win(1, caption(folder), 5), win(2, caption('/x'), 77), win(3, f'My "Repo" - {app}'), win(4, 'none'), win(5, caption(folder), 77, False)]
            self.assertEqual(self.kwin_run(probe, wins), [None, [True] * 5, [f'mark-1 3 1 1 1 {caption("/x")}']], 'the probe only prints')
            self.assertEqual(self.kwin_run(probe, [win(1, 'none')]), [None, [True], ['mark-1 0 0 0 0 ']])

    def test_notify_extension(self):
        """.claude/vscode/terminal-focus: its package.json (publisher.name is notify.py's EXTENSION, onUri activation, its main
        file, no dependencies or scripts), and, run by Node.js with a stub vscode module, the URI handler showing the
        terminal whose shell is the pid or its nearest ancestor, nothing for another path, a bad pid, pid 1 or a pid no
        terminal's shell is an ancestor of; the same cases with /proc unreadable, through `ps` (macOS). A window without the
        terminal and a folder in the URI leaves the request; focused, it opens the folder at once and once more only when it
        regains focus after losing it, within WAIT; unfocused, once it gains focus, else after WAIT; the folder's window shows
        the terminal when it gains focus, not on losing it, and removes the request; a request older than FRESH is ignored."""
        ext = ROOT / '.claude/vscode/terminal-focus'
        pkg = json.loads((ext / 'package.json').read_text())
        n = self.imported()
        self.assertEqual(f"{pkg['publisher']}.{pkg['name']}", n.EXTENSION)
        self.assertEqual((pkg['activationEvents'], pkg['main'], 'vscode' in pkg['engines']), (['onUri', 'onStartupFinished'], './extension.js', True))
        self.assertFalse({'dependencies', 'devDependencies', 'scripts'} & set(pkg))
        self.assertEqual(pkg['capabilities'], {'untrustedWorkspaces': {'supported': True}, 'virtualWorkspaces': True},
                         'it runs no workspace code, so a Restricted Mode window keeps it (live check 2026-11-24)')
        self.assertEqual(sorted(p.name for p in ext.iterdir()), ['extension.js', 'package.json'])
        if not NODE:
            self.skipTest('Node.js missing: the handler is untested')
        stub = self.tmp / 'node_modules/vscode'
        stub.mkdir(parents=True)
        (stub / 'index.js').write_text('const m = module.exports = {shown: [], listeners: [], commands: {calls: [], executeCommand(...a) { m.commands.calls.push(a); }},'
                                       'Uri: {parse: s => ({parsed: s})}, window: {state: {focused: true}, terminals: [], registerUriHandler(h) { m.handler = h; return {dispose() {}}; },'
                                       'onDidChangeWindowState(f) { m.listeners.push(f); return {dispose() { m.listeners.splice(m.listeners.indexOf(f) >>> 0, 1); }}; }},'
                                       'focusChanged: s => Promise.all(m.listeners.slice().map(f => f(s)))};\n')
        harness = ('if (process.env.NOPROC) { const fs = require("fs"), read = fs.readFileSync;'  # macOS: no /proc, ancestors by `ps`
                   'fs.readFileSync = (p, ...a) => { if (String(p).startsWith("/proc/")) throw new Error("no /proc"); return read(p, ...a); }; }'
                   'const vscode = require("vscode"); const ext = require(process.argv[1]); const ctx = {subscriptions: []}; ext.activate(ctx);'
                   'const sub = s => String(s).replace("SELF", process.pid).replace("PARENT", process.ppid);'
                   '(async () => { const res = [];'
                   'for (const [terms, path, query] of JSON.parse(process.argv[2])) {'
                   'vscode.window.terminals = terms.map(t => ({processId: Promise.resolve(t === null ? undefined : Number(sub(t))),'
                   'show() { vscode.shown.push(t); }})); vscode.shown.length = 0;'
                   'res.push([await vscode.handler.handleUri({path, query: sub(query)}), [...vscode.shown]]); }'
                   'console.log(JSON.stringify([ctx.subscriptions.length, res])); })();')
        cases = [(['PARENT'], '/focus', 'pid=SELF', [True, ['PARENT']]), (['PARENT', 'SELF'], '/focus', 'pid=SELF', [True, ['SELF']]),
                 ([None, 'SELF'], '/focus', 'pid=SELF', [True, ['SELF']]), (['99999999'], '/focus', 'pid=SELF', [False, []]),
                 (['PARENT'], '/other', 'pid=SELF', [False, []]), (['PARENT'], '/focus', 'pid=abc', [False, []]),
                 (['1'], '/focus', 'pid=1', [False, []]), (['PARENT'], '/focus', '', [False, []])]
        for noproc in ('', '1'):
            r = subprocess.run([NODE, '-e', harness, str(ext / 'extension.js'), json.dumps([c[:3] for c in cases])], capture_output=True, text=True,
                               timeout=60, env=self.env | {'NODE_PATH': str(self.tmp / 'node_modules'), 'NOPROC': noproc})
            self.assertEqual((r.returncode, r.stderr), (0, ''), noproc)
            self.assertEqual(json.loads(r.stdout), [2, [c[3] for c in cases]], 'the ps fallback' if noproc else '/proc')
        relay = ('const vscode = require("vscode"); const ext = require(process.argv[1]); const fs = require("fs"), dir = process.argv[2];'
                 'const ctx = {subscriptions: [], globalStorageUri: {fsPath: dir}}; ext.activate(ctx);'
                 'const term = id => ({processId: Promise.resolve(id), show() { vscode.shown.push(id); }}), out = {}, calls = vscode.commands.calls;'
                 'const handle = () => vscode.handler.handleUri({path: "/focus", query: "pid=" + process.pid + process.argv[3]});'
                 'const wait = ms => new Promise(r => setTimeout(r, ms));'
                 '(async () => { vscode.window.terminals = [term(99999999)];'  # a focused window without the terminal: the folder at once
                 'out.handled = await handle();'
                 'out.request = JSON.parse(fs.readFileSync(dir + "/request.json", "utf8")).pid === process.pid; out.opened = calls.splice(0);'
                 'await vscode.focusChanged({focused: true}); out.noBlur = calls.length;'  # focus without losing it first: not again
                 'await vscode.focusChanged({focused: false}); await vscode.focusChanged({focused: true}); out.back = calls.splice(0).length;'
                 'await vscode.focusChanged({focused: false}); await vscode.focusChanged({focused: true}); out.twice = calls.length;'
                 'await handle(); calls.length = 0; await wait(2100); out.kept = vscode.listeners.length;'  # never refocused: its listener goes
                 'await vscode.focusChanged({focused: false}); await vscode.focusChanged({focused: true}); out.late = calls.length;'
                 'vscode.window.terminals = [term(process.ppid)];'  # the folder's window, holding the terminal of an ancestor
                 'await vscode.focusChanged({focused: false}); out.unfocused = [...vscode.shown];'
                 'await vscode.focusChanged({focused: true}); out.shown = [...vscode.shown]; out.left = fs.existsSync(dir + "/request.json");'
                 'fs.writeFileSync(dir + "/request.json", JSON.stringify({pid: process.pid, at: Date.now() - 16000})); vscode.shown.length = 0;'
                 'await vscode.focusChanged({focused: true}); out.stale = [...vscode.shown];'
                 'vscode.window.terminals = [term(99999999)]; vscode.window.state.focused = false;'  # an unfocused one: the folder once it gains focus
                 'await handle(); await wait(450); out.beforeFocus = calls.length;'
                 'await vscode.focusChanged({focused: false}); out.onBlur = calls.length;'
                 'await vscode.focusChanged({focused: true}); out.onFocus = calls.splice(0).length;'
                 'await wait(2100); out.once = calls.length; out.listeners = vscode.listeners.length;'
                 'await handle(); await wait(1700); out.beforeWait = calls.length;'  # never focused: after WAIT
                 'await wait(500); out.afterWait = calls.splice(0).length; out.left2 = vscode.listeners.length;'
                 'console.log(JSON.stringify(out)); })();')
        # notify.py's folder parameter for C:\x\My Repo\C#, its query decoded once as VS Code's URI.parse does (mainImpl.js 1.140)
        n = self.imported()
        query = n.unquote('&folder=' + n.quote('file://' + n.uri_path(n.PureWindowsPath(r'C:\x\My Repo\C#')).removeprefix('//'), safe=''))
        r = subprocess.run([NODE, '-e', relay, str(ext / 'extension.js'), str(self.tmp / 'storage/compprog.terminal-focus'), query], capture_output=True, text=True,
                           timeout=60, env=self.env | {'NODE_PATH': str(self.tmp / 'node_modules')})
        self.assertEqual((r.returncode, r.stderr), (0, ''))
        self.assertEqual(json.loads(r.stdout), {'handled': False, 'request': True, 'opened': [['vscode.openFolder', {'parsed': 'file:///C:/x/My%20Repo/C%23'},
                                                {'forceNewWindow': True}]], 'noBlur': 0, 'back': 1, 'twice': 0, 'kept': 1, 'late': 0,
                                                'unfocused': [], 'shown': [os.getpid()], 'left': False, 'stale': [],
                                                'beforeFocus': 0, 'onBlur': 0, 'onFocus': 1, 'once': 0, 'listeners': 1, 'beforeWait': 0, 'afterWait': 1, 'left2': 1},
                         'a focused window opens the folder at once and again only on regaining focus within WAIT; an unfocused one once it '
                         'gains focus, once, else after WAIT; its listener goes')

    def test_notify_end(self):
        """The helper ended by SIGTERM or SIGHUP (/reap, the SessionEnd hook) before a click sends SIGINT to its notify-send,
        which closes the notification, exits 0 and runs no launcher."""
        self.write('.claude/hooks/notify.py', (HOOKS / 'notify.py').read_text())
        self.fakes()
        hold, log, code_log = self.tmp / 'hold-send', self.tmp / 'hold.log', self.tmp / 'code.log'
        hold.write_text(f'#!{sys.executable}\nimport os, signal, sys, time\n'
                        'def got(sig, _):\n    open(os.environ["HOLD_LOG"], "a").write(signal.Signals(sig).name + "\\n")\n    sys.exit(0)\n'
                        'for s in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):\n    signal.signal(s, got)\n'
                        'open(os.environ["HOLD_LOG"] + ".pid", "w").write(str(os.getpid()))\ntime.sleep(60)\n')
        hold.chmod(0o755)
        for sig in (signal.SIGTERM, signal.SIGHUP):
            for f in (log, Path(f'{log}.pid')):
                f.unlink(missing_ok=True)
            p = subprocess.Popen([sys.executable, str(self.repo / '.claude/hooks/notify.py'), '--click', str(hold), str(self.tmp / 'bin/code'),
                                  str(self.repo), 'h', 'b', 'App', '', 'ed://x'], env=self.env | {'HOLD_LOG': str(log), 'CODE_LOG': str(code_log)},
                                 start_new_session=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            try:
                self.assertTrue(self.wait(Path(f'{log}.pid')) and Path(f'{log}.pid').read_text(), 'the helper notifies')
                p.send_signal(sig)
                self.assertEqual(p.wait(timeout=15), 0, sig)
                self.assertEqual(log.read_text() if log.exists() else '', 'SIGINT\n', 'notify-send gets SIGINT')
                self.assertFalse(code_log.exists() or Path(f'{code_log}.url').exists(), 'no launcher')
            finally:
                p.kill()
                with contextlib.suppress(OSError, ValueError):
                    os.kill(int(Path(f'{log}.pid').read_text()), signal.SIGKILL)

    def test_notify_title_setting(self):
        """The project's .vscode/settings.json sets window.title to VS Code 1.140's default with ${rootName} replaced by
        ${rootPath} (the KWin match's full path), and git tracks it; the default is checked against the bundle of the
        VS Code whose launcher is on PATH, when there is one."""
        self.assertEqual(json.loads((ROOT / '.vscode/settings.json').read_text()), {'window.title': DEFAULT_TITLE.replace('${rootName}', '${rootPath}')})
        r = subprocess.run(['git', 'check-ignore', '-q', '.vscode/settings.json'], cwd=ROOT, env=self.env, capture_output=True)
        self.assertEqual(r.returncode, 1, 'not ignored')
        code = shutil.which('code')  # the bin folder of a deb, rpm or tarball installation, else Arch's and the AUR's folders
        bundle = next((b for d in ([Path(os.path.realpath(code)).parents[1]] if code else []) + [Path('/usr/share/code'), Path('/opt/visual-studio-code')]
                       if (b := d / 'resources/app/out/vs/workbench/workbench.desktop.main.js').is_file()), None)
        if not bundle:
            self.skipTest("no VS Code bundle: the default title is VS Code 1.140's as recorded")
        self.assertIn(f'"{DEFAULT_TITLE}"', bundle.read_text(encoding='utf-8', errors='replace'), 'the default window.title of the installed VS Code')

    def test_notify_wsl_drive(self):
        """wsl_drive(): <[automount] root>/c from wsl.conf, quoted or with a comment, else /mnt/c (no file, no key, no section
        header, another section); WIN_USERS and WIN_PS sit on it."""
        n = self.imported()
        conf = self.tmp / 'wsl.conf'
        for text, want in (('[automount]\nroot = /win/\n', '/win/c'), ('[automount]\nenabled = true\nroot = "/drives" # mine\n', '/drives/c'),
                           ('[network]\nroot = /x/\n', '/mnt/c'), ('root = /x/\n', '/mnt/c'), ('[automount]\noptions = "metadata"\n', '/mnt/c'), (None, '/mnt/c')):
            conf.unlink(missing_ok=True)
            if text is not None:
                conf.write_text(text)
            self.assertEqual(n.wsl_drive(conf), Path(want), text)
        self.assertEqual((n.WIN_USERS.name, n.WIN_PS.parts[-5:]), ('Users', ('Windows', 'System32', 'WindowsPowerShell', 'v1.0', 'powershell.exe')))

    def test_notify_winhost(self):
        """winhost() on a fake Windows process table: the first ancestor whose executable has resources/app/product.json
        (Code.exe's pty host), the shell below it, the topmost ancestor on the same executable, the editor's flags from its
        processes' arguments, a parent that started after its child (a reused id) not climbed; {} without one, at pid 0, or when reading an executable fails; host() takes this route on
        win32. winprocs(), winexe(), winargs() and winborn() run only on Windows (live checks B and D)."""
        n = self.imported()
        app = self.tmp / 'Microsoft VS Code'
        (app / 'resources/app').mkdir(parents=True)
        (app / 'resources/app/product.json').write_text('{}')
        code = str(app / 'Code.exe')
        exe = {10: 'C:/node.exe', 9: 'C:/pwsh.exe', 8: code, 7: code, 6: 'C:/explorer.exe'}.get
        parents = {10: 9, 9: 8, 8: 7, 7: 6, 6: 0}
        self.assertEqual(n.winhost(10, parents, exe), {'app': app / 'resources/app', 'shell': 9, 'pid': 7, 'data': None, 'extensions': None})
        self.assertEqual(n.winhost(10, {10: 9, 9: 0}, exe), {}, 'no editor among the ancestors')
        args = {8: ['Code.exe', '--type=ptyHost', '--user-data-dir=C:/near'], 7: ['Code.exe', '--user-data-dir', 'D:/far', '--extensions-dir', 'E:/ext'],
                10: ['node.exe', '--extensions-dir=X:/not the editor']}
        self.assertEqual(n.winhost(10, parents, exe, args.get), {'app': app / 'resources/app', 'shell': 9, 'pid': 7, 'data': 'C:/near', 'extensions': 'E:/ext'},
                         "the first --user-data-dir and --extensions-dir from the editor's process upward, either form; not below it")
        self.assertEqual(n.winhost(10, parents, exe, {7: None}.get)['data'], None, 'arguments unreadable (an elevated editor)')
        self.assertIsNone(n.winargs(os.getpid()), 'no Windows API here: None, never an error')
        born = {10: 5, 9: 4, 8: 3, 7: 2, 6: 1}
        whole = {'app': app / 'resources/app', 'shell': 9, 'pid': 7, 'data': None, 'extensions': None}
        self.assertEqual(n.winhost(10, parents, exe, born=born.get), whole, 'each parent started before its child')
        self.assertEqual(n.winhost(10, parents, exe, born={**born, 7: 50}.get), whole | {'pid': 8},
                         "a parent id reused by a later process: the climb to the main process stops below it")
        self.assertEqual(n.winhost(10, parents, exe, born={**born, 8: 9}.get), {}, 'the editor started after the shell: not its parent')
        self.assertEqual(n.winhost(10, parents, exe, born={10: 5}.get), whole, 'unreadable start times keep the parent')
        self.assertIsNone(n.winborn(os.getpid()), 'no Windows API here: None, never an error')
        self.assertEqual(n.winhost(10, {10: 10}, exe), {}, 'a loop ends after HOPS')
        def broken(pid):
            raise OSError('denied')
        self.assertEqual(n.winhost(10, parents, broken), {}, 'a failing read')
        win = self.tmp / 'VS Code 1.140'  # Windows: product.json under a folder named by the build's commit (live check B)
        for build, at in (('5a1b2c3d4e', 1000), ('69047191fc', 2000)):
            (win / build / 'resources/app').mkdir(parents=True)
            (win / build / 'resources/app/product.json').write_text('{}')
            os.utime(win / build / 'resources/app/product.json', (at, at))
        (win / 'bin').mkdir()
        self.assertEqual(n.product_dir(win / 'Code.exe'), win / '69047191fc/resources/app', 'the newest build')
        self.assertEqual(n.product_dir(app / 'Code.exe'), app / 'resources/app', 'resources/app beside the executable first')
        self.assertIsNone(n.product_dir(self.tmp / 'Windows/explorer.exe'))
        code = str(win / 'Code.exe')
        self.assertEqual(n.winhost(10, parents, {10: 'C:/py.exe', 9: 'C:/pwsh.exe', 8: code, 7: code, 6: 'C:/explorer.exe'}.get)['app'],
                         win / '69047191fc/resources/app')
        n.SYSTEM, seen = 'win32', []
        n.winhost = lambda pid=None: seen.append(pid) or {'app': 1}
        self.assertEqual((n.host(5), seen), ({'app': 1}, [5]), 'host() on win32')

    def test_notify_folders(self):
        """folders() and window() on Linux with the editor's --user-data-dir: the storage.json windows, a folder window and an
        untitled workspace window (Workspaces/<id>/workspace.json through its configURIPath, `path` and `uri` entries), a
        broken listed workspace skipped, a closed untitled workspace whose file stays never read; also under the platform
        folder by name; window folders and the working directory compared with symbolic links resolved, the folder returned
        as the window lists it."""
        n = self.imported()
        n.SYSTEM = 'linux'
        P = n.PurePosixPath
        udd, proj, other, link = self.tmp / 'udd', self.tmp / 'proj', self.tmp / 'other', self.tmp / 'link'
        for d in (proj / 'sub/deep', other):
            d.mkdir(parents=True)
        link.symlink_to(proj)
        uri = lambda f: 'file://' + quote(str(f))

        def windows(*folders, at=udd, spaces=()):
            (at / 'User/globalStorage').mkdir(parents=True, exist_ok=True)
            listed = [{'folder': uri(f)} for f in folders] + [{'workspaceIdentifier': {'id': w, 'configURIPath': uri(at / 'Workspaces' / w / 'workspace.json')}}
                                                              for w in spaces]
            (at / 'User/globalStorage/storage.json').write_text(json.dumps({'windowsState': {'openedWindows': listed}}))
        for wid, text in (('1700000000002', json.dumps({'folders': [{'path': str(proj / 'sub')}, {'uri': uri(link)}]})),
                          ('1700000000001', '{"folders": ['), ('1700000000004', json.dumps({'folders': [{'path': str(proj / 'sub/deep')}]}))):
            (udd / 'Workspaces' / wid).mkdir(parents=True)
            (udd / 'Workspaces' / wid / 'workspace.json').write_text(text)
        windows(other, spaces=('1700000000001', '1700000000002'))  # ...4 is closed: its file stays, unlisted
        self.assertEqual(n.folders(None, str(udd)), [P(other), P(proj / 'sub'), P(link)])
        self.assertEqual(n.folders('Code', str(udd)), [P(other), P(proj / 'sub'), P(link)], 'data wins over the name')
        self.assertEqual(n.window(str(proj / 'sub/deep'), None, str(udd)), P(proj / 'sub'), 'the deepest open one, an untitled workspace')
        self.assertEqual(n.window(str(proj / 'sub/deep'), None, str(udd), file=True), P(proj / 'sub'), 'an untitled workspace has no file to open')
        self.assertEqual(n.window(str(proj), None, str(udd)), P(link), 'a symlinked window folder, as listed')
        windows(link)
        self.assertEqual(n.window(str(link / 'sub'), None, str(udd)), P(link), 'the working directory through the link; no workspace listed')
        windows(proj)
        self.assertEqual(n.window(str(link / 'sub'), None, str(udd)), P(proj), 'a window on the real path')
        config = self.tmp / 'config'
        (config / 'Code/Workspaces/9').mkdir(parents=True)
        (config / 'Code/Workspaces/9/workspace.json').write_text(json.dumps({'folders': [{'path': str(proj)}]}))
        windows(other, at=config / 'Code', spaces=('9',))
        with unittest.mock.patch.dict(os.environ, {'XDG_CONFIG_HOME': str(config)}):
            os.environ.pop('VSCODE_PORTABLE', None)
            os.environ.pop('VSCODE_APPDATA', None)
            self.assertEqual(n.folders('Code'), [P(other), P(proj)])
            self.assertEqual(n.folders(None), [], 'no name and no data')
        n.SYSTEM, n.WIN_USERS = 'wsl', self.tmp / 'mnt/c/Users'  # an untitled workspace of a WSL window lives on Windows
        data = n.WIN_USERS / 'bob/AppData/Roaming/Code'
        (data / 'Workspaces/17').mkdir(parents=True)
        (data / 'Workspaces/17/workspace.json').write_text(json.dumps({'folders': [{'uri': 'vscode-remote://wsl%2Bubuntu/work/bob/proj'},
                                                                                   {'uri': 'file:///c%3A/x'}]}))
        windows(at=data)
        state = {'lastActiveWindow': {'workspaceIdentifier': {'id': '17', 'configURIPath': 'file:///c%3A/Users/bob/AppData/Roaming/Code/Workspaces/17/workspace.json'}},
                 'openedWindows': [{'folder': 'file:///c%3A/Users/bob/win'}]}
        (data / 'User/globalStorage/storage.json').write_text(json.dumps({'windowsState': state}))
        with unittest.mock.patch.dict(os.environ, {'WSL_DISTRO_NAME': 'Ubuntu'}):
            self.assertEqual(n.folders('Code', str(data)), [P('/work/bob/proj')], 'Windows folders are no WSL working directory')
        drive = n.WIN_USERS.parent.parent
        for u, want in (('file:///C%3A/Users/b', drive / 'c/Users/b'), ('file:///d:', drive / 'd'), ('file:///home/x', None),
                        ('file://server/c%3A/x', None), ('vscode-remote://wsl%2Bubuntu/c%3A/x', None)):
            self.assertEqual(n.wsl_file(u), want and P(want), u)
        n.SYSTEM = 'linux'
        self.assertIsNone(n.wsl_file('file:///c%3A/x'), 'WSL only')

    def test_guard_git(self):
        """guard.py refuses Edit, Write and NotebookEdit of any path through a .git file or folder, in this or another
        repository, absolute or relative, also through a symlink; reads and look-alike names pass."""
        self.write('.claude/hooks/guard.py', (HOOKS / 'guard.py').read_text())
        git(self.repo, 'worktree', 'add', '-q', '-b', 'P900-SUP32-g', str(self.tmp / 'wt'))
        other = self.tmp / 'other'
        git(self.tmp, 'init', '-q', str(other))
        (self.tmp / 'link').symlink_to(self.repo / '.git')
        deny = {'hookSpecificOutput': {'hookEventName': 'PreToolUse', 'permissionDecision': 'deny', 'permissionDecisionReason':
                "Git's own files (.git/) are not edited with Edit or Write: their config names the hooks that guard pushes and "
                'commits (CLAUDE.md, Git). Use git commands instead.'}}

        def run(tool, path, key='file_path'):
            return self.hook('guard.py', payload={'tool_name': tool, 'tool_input': {key: str(path)}})
        for tool in ('Edit', 'Write'):
            for path in (self.repo / '.git/config', self.repo / '.git/hooks/pre-push', '.git/config', self.tmp / 'wt/.git',
                         other / '.git/config', self.tmp / 'link/config', self.repo / '.git/new/file', self.repo / '.GIT/config',
                         self.repo / '.Git/hooks/x'):
                self.assertEqual(run(tool, path), deny, (tool, path))
        self.assertEqual(run('NotebookEdit', self.repo / '.git/x.ipynb', 'notebook_path'), deny)
        for tool, path in (('Read', self.repo / '.git/config'), ('Edit', self.repo / '.github/workflows/x.yml'),
                           ('Write', self.repo / '.gitignore'), ('Edit', self.repo / 'a.git/config'), ('Write', other / 'x.txt')):
            self.assertIsNone(run(tool, path), (tool, path))

    def test_notify_platforms(self):
        """command() with stubbed notifiers on macOS, Windows and WSL; detect(); the helper's wait bound."""
        spec = importlib.util.spec_from_file_location('notify_copy', HOOKS / 'notify.py')
        n = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(n)
        n.DEADLINE = time.monotonic() + 3600  # the hook's 9 s count from the import; these calls run far longer
        n.PROC = self.tmp / 'no proc'  # host() finds nothing, not the editor whose terminal may run these tests
        which = n.shutil.which
        tools = {}
        n.shutil = types.SimpleNamespace(which=tools.get)
        head, body = 'Claude Code · <a&b> "q" it\'s ’‘‚‛ ü', 'waits for your input'
        folder = self.tmp / 'My Repo'
        folder.mkdir()
        n.WIN_USERS = self.tmp / 'Users'

        def argv(system, focus=True, product=None, app=None, **t):
            n.SYSTEM = system
            tools.clear()
            tools.update(t)
            return n.command(head, body, str(folder), (dict(n.VSCODE) if product is None else product) if focus else None, app)

        def script(a):
            self.assertEqual(a[1:4], ['-NoProfile', '-NonInteractive', '-EncodedCommand'])
            s = base64.b64decode(a[4]).decode('utf-16-le')
            literal = s.split("LoadXml('")[1].split("')\n")[0]
            self.assertTrue(literal.isascii() and "'" not in literal, literal)
            return s
        tn = [head, '-message', body, '-group', head]
        self.assertEqual(argv('darwin', **{'terminal-notifier': '/tn'}),
                         ['/tn', '-title', *tn, '-execute', f"open -b com.microsoft.VSCode '{folder}'"])
        self.assertEqual(argv('darwin', False, **{'terminal-notifier': '/tn'}), ['/tn', '-title', *tn])
        cursor = {'nameShort': 'Cursor', 'applicationName': 'cursor', 'urlProtocol': 'cursor'}
        bundle = self.tmp / 'Cursor B.app'
        self.assertEqual(argv('darwin', product=cursor, app=bundle / 'Contents/Resources/app', **{'terminal-notifier': '/tn'}),
                         ['/tn', '-title', *tn, '-execute', f"open -a '{bundle}' '{folder}'"], 'another editor: its own app bundle')
        for product, app, why in ((cursor, self.tmp / 'Cursor/resources/app', 'an editor outside an app bundle'),
                                  ({}, bundle / 'Contents/Resources/app', 'a product.json without nameShort')):
            self.assertEqual(argv('darwin', product=product, app=app, **{'terminal-notifier': '/tn'}), ['/tn', '-title', *tn], why)
        self.assertEqual(argv('darwin', osascript='/osa'), ['/osa', '-e', 'on run argv', '-e', 'display notification (item 2 of argv) with title '
                                                             '(item 1 of argv)', '-e', 'end run', head, body], 'osascript: no click')
        self.assertFalse(argv('darwin'), 'macOS without a notifier')
        xml = ('<text>Claude Code &#183; &lt;a&amp;b&gt; &quot;q&quot; it&#39;s &#8217;&#8216;&#8218;&#8219; &#252;</text>'
               '<text>waits for your input</text>')
        with unittest.mock.patch.dict(os.environ, {'WSL_DISTRO_NAME': 'Ubuntu 24'}):
            s = script(argv('wsl', **{'powershell.exe': '/ps.exe'}))
            self.assertIn(f'<toast activationType="protocol" launch="vscode://vscode-remote/wsl+Ubuntu%2024{quote(str(folder))}">', s)
            self.assertIn(xml, s)
            self.assertIn("CreateToastNotifier('" + n.AUMID + "')", s)
            self.assertIn('<toast><visual>', script(argv('wsl', False, **{'powershell.exe': '/ps.exe'})))
            self.assertIn(f'launch="cursor://vscode-remote/wsl+Ubuntu%2024{quote(str(folder))}"',
                          script(argv('wsl', product=cursor, **{'powershell.exe': '/ps.exe'})))
            self.assertIn('<toast><visual>', script(argv('wsl', product={'nameShort': 'Cursor'}, **{'powershell.exe': '/ps.exe'})),
                          'no urlProtocol: no click')
            n.WIN_PS = self.tmp / 'powershell.exe'
            self.assertFalse(argv('wsl'), 'WSL without powershell.exe')
            n.WIN_PS.touch()
            self.assertEqual(argv('wsl')[0], str(n.WIN_PS), 'the fixed Windows path from WSL')
        with unittest.mock.patch.dict(os.environ, {}):
            os.environ.pop('WSL_DISTRO_NAME', None)
            self.assertIn('<toast><visual>', script(argv('wsl', **{'powershell.exe': '/ps.exe'})), 'no distro name: no click')
        a = argv('win32', powershell='/ps')
        self.assertEqual(a[0], '/ps')
        self.assertIn(f'launch="vscode://file{quote(str(folder))}"', script(a))
        self.assertEqual(n.link(n.PureWindowsPath('C:/Users/x/My Repo'), n.VSCODE), 'vscode://file/C:/Users/x/My%20Repo')
        self.assertEqual(n.link(n.PureWindowsPath('C:/x'), {'urlProtocol': 'cursor'}), 'cursor://file/C:/x')
        self.assertIsNone(n.link(n.PureWindowsPath('C:/x'), {}))
        self.assertEqual(n.link(n.PureWindowsPath('//server/share/My Repo'), n.VSCODE), 'vscode://file//server/share/My%20Repo',
                         'a UNC folder keeps its server')
        self.assertIn(f'launch="cursor://file{quote(str(folder))}"', script(argv('win32', product=cursor, powershell='/ps')))
        self.assertIn('<toast><visual>', script(argv('win32', product={}, powershell='/ps')), 'no urlProtocol: no click')
        exts = self.tmp / 'exts'
        (exts / 'compprog.terminal-focus-0.0.1').mkdir(parents=True)
        win = lambda found: script(n.command(head, body, str(folder), dict(n.VSCODE), None, found))
        held = [n.PureWindowsPath('C:/Users/x/My Repo')]
        with unittest.mock.patch.object(n, 'window', lambda cwd, name, data=None, file=False: held[0] if file else None):
            self.assertIn('launch="vscode://file/C:/Users/x/My%20Repo"', win({'shell': 4242, 'extensions': str(exts)}), 'no package.json: not installed')
            (exts / 'compprog.terminal-focus-0.0.1/package.json').write_text('{}')
            self.assertIn('launch="vscode://compprog.terminal-focus/focus?pid=4242&amp;folder=file%3A%2F%2F%2FC%3A%2FUsers%2Fx%2FMy%2520Repo"',
                          win({'shell': 4242, 'extensions': str(exts)}), 'the extension shows the terminal in the window holding the folder')
            held[0] = n.PureWindowsPath('C:/src/C# x.code-workspace')
            self.assertIn('folder=file%3A%2F%2F%2FC%3A%2Fsrc%2FC%2523%2520x.code-workspace"', win({'shell': 4242, 'extensions': str(exts)}),
                          "a '#' quoted twice, since VS Code decodes the query once; a saved workspace's file")
            held[0] = n.PureWindowsPath('//srv/share/x')
            self.assertIn('folder=file%3A%2F%2Fsrv%2Fshare%2Fx"', win({'shell': 4242, 'extensions': str(exts)}), 'a UNC folder as its server')
        self.assertFalse(argv('win32'), 'Windows without PowerShell')
        for env, release, plat, want in (({}, '6.6.87.2-microsoft-standard-WSL2', 'linux', 'wsl'), ({'WSL_DISTRO_NAME': 'U'}, '7.2', 'linux', 'wsl'),
                                         ({}, '7.2.11-arch1-1', 'linux', 'linux'), ({}, '24.1.0', 'darwin', 'darwin'), ({}, '10', 'win32', 'win32')):
            self.assertEqual(n.detect(env, release, plat), want, (env, release, plat))
        self.notify_activate(n, tools)
        n.shutil.which = which
        self.fakes()
        code_log = self.tmp / 'code.log'
        n.WAIT = 1
        with unittest.mock.patch.dict(os.environ, {'NOTIFY_LOG': str(self.tmp / 'n.log'), 'NOTIFY_SLEEP': '30', 'CODE_LOG': str(code_log)}):
            start = time.monotonic()
            n.click(str(self.tmp / 'bin/notify-send'), str(self.tmp / 'bin/code'), str(folder), 'h', 'b')
            self.assertLess(time.monotonic() - start, 8, 'the helper gives up after WAIT')
        self.assertFalse(code_log.exists(), 'no click before the wait ended')
        self.notify_editor(n, folder)
        self.notify_title(n)
        self.notify_workspace(n, folder)

    def notify_activate(self, n, tools):
        """activate() on KDE: a one-shot KWin script over gdbus, else qdbus6, else qdbus: loaded, run by the id loadScript
        returns (none on an error reply), unloaded, its file removed; the script names the folder's name and the app as JSON
        strings. Nothing outside KDE (XDG_CURRENT_DESKTOP's entries), without an app and a pid, or without a D-Bus tool."""
        fake, log = self.tmp / 'dbus', self.tmp / 'kwin.log'
        fake.write_text(FAKE_DBUS)
        fake.chmod(0o755)
        folder = '/w/My "Repo" – x'

        def run(desktop, app='Visual Studio Code', out='(7,)', pid=None, **t):
            tools.clear()
            tools.update({k: str(v) for k, v in t.items()})
            for f in (log, Path(f'{log}.js')):
                f.unlink(missing_ok=True)
            with unittest.mock.patch.dict(os.environ, {'XDG_CURRENT_DESKTOP': desktop, 'KWIN_LOG': str(log), 'KWIN_OUT': out,
                                                       'CODE_LOG': str(self.tmp / 'none')}):
                n.activate(folder, app, pid)
            return [line.split('\0')[:-1] for line in log.read_text().split('\n') if line] if log.exists() else []
        for desktop, out, t, prefix, method, script in (
                ('KDE', '(7,)', {'gdbus': fake, 'qdbus6': '/x'}, ['call', '--session', '--dest', 'org.kde.KWin', '--object-path'], ['--method'], 7),
                ('ubuntu:KDE', '12', {'qdbus6': fake, 'qdbus': '/x'}, ['org.kde.KWin'], [], 12),
                ('KDE', '12', {'qdbus': fake}, ['org.kde.KWin'], [], 12),
                ('KDE', 'Error: no such object', {'gdbus': fake}, ['call', '--session', '--dest', 'org.kde.KWin', '--object-path'], ['--method'], None)):
            calls = run(desktop, out=out, **t)
            path, plugin = calls[0][-2:]
            want = [[*prefix, '/Scripting', *method, 'org.kde.kwin.Scripting.loadScript', path, plugin],
                    *([[*prefix, f'/Scripting/Script{script}', *method, 'org.kde.kwin.Script.run']] if script else []),
                    [*prefix, '/Scripting', *method, 'org.kde.kwin.Scripting.unloadScript', plugin]]
            self.assertEqual(calls, want, (desktop, out, t))
            self.assertEqual((plugin, Path(path).exists()), (f'claude-notify-{os.getpid()}', False), 'the script file is removed')
            js = Path(f'{log}.js').read_text()
            self.assertIn('const path = " - " + ' + json.dumps(folder) + ' + " - ", name = " - " + ' + json.dumps('My "Repo" – x')
                          + ' + " - ", app = "Visual Studio Code", pid = 0, mark = "";', js)
            self.assertIn('workspace.activeWindow = best', js)
        for desktop, app, t in (('GNOME', 'Visual Studio Code', {'gdbus': fake}), ('KDE', '', {'gdbus': fake}), ('', 'Code', {'gdbus': fake}),
                                ('KDEx:GNOME', 'Code', {'gdbus': fake}), ('KDE', 'Code', {})):
            self.assertEqual(run(desktop, app, **t), [], (desktop, app, t))
        self.assertEqual(len(run('KDE', '', pid='4242', gdbus=fake)), 3, 'a pid alone activates')
        self.assertIn(', app = "", pid = 4242, mark = "";', Path(f'{log}.js').read_text())

    def notify_editor(self, n, folder):
        """editor(), cli(), folders() and window() of the imported module: the product found from the askpass script,
        its launcher, and the window folders of Windows (file URIs) and WSL (this distribution's vscode-remote URIs)."""
        app = self.tmp / 'app'
        (app / 'resources/app/extensions/git/dist').mkdir(parents=True)
        product = app / 'resources/app/product.json'
        main = str(app / 'resources/app/extensions/git/dist/askpass-main.js')
        cursor = {'nameShort': 'Cursor', 'applicationName': 'cursor', 'urlProtocol': 'cursor'}
        ask = {'VSCODE_GIT_ASKPASS_MAIN': main}
        for env, text, want in (({}, None, n.VSCODE), (ask, None, n.VSCODE), ({'VSCODE_GIT_ASKPASS_MAIN': '/a.js'}, None, n.VSCODE),
                                (ask, 'not json', n.VSCODE), (ask, '[1]', n.VSCODE), (ask, json.dumps(cursor), cursor),
                                (ask, json.dumps(cursor | {'nameLong': 'Cursor', 'urlProtocol': 5, 'darwinBundleIdentifier': 'x'}),
                                 {'nameShort': 'Cursor', 'applicationName': 'cursor', 'nameLong': 'Cursor'}),
                                (ask, json.dumps(cursor | {'nameLong': 7}), cursor),
                                (ask, json.dumps({'nameShort': '', 'applicationName': 'cursor', 'urlProtocol': None}), {'applicationName': 'cursor'}),
                                (ask, '{}', {})):
            product.unlink(missing_ok=True)
            if text is not None:
                product.write_text(text)
            with unittest.mock.patch.dict(os.environ, env):
                if not env:
                    os.environ.pop('VSCODE_GIT_ASKPASS_MAIN', None)
                got, where, found = n.editor()
            self.assertEqual(found, {})
            self.assertEqual(got, want, (env, text))
            self.assertEqual(where, app / 'resources/app' if text and text.startswith('{') else None, (env, text))
        n.shutil.which = {'cursor': '/path/cursor'}.get
        self.assertEqual(n.cli(cursor, None), '/path/cursor')
        self.assertIsNone(n.cli(n.VSCODE, app / 'resources/app'))
        n.shutil.which = {'cursor': '/path/cursor', 'code': '/path/code'}.get
        self.assertIsNone(n.cli({}, app / 'resources/app'), 'no applicationName')
        for d in (app / 'bin', app / 'resources/app/bin'):
            d.mkdir()
            (d / 'cursor').mkdir()
            self.assertEqual(n.cli(cursor, app / 'resources/app'), '/path/cursor', 'a folder named like the launcher')
            (d / 'cursor').rmdir()
            (d / 'cursor').write_text('#!/bin/sh\n')
            (d / 'cursor').chmod(0o644)
            self.assertEqual(n.cli(cursor, app / 'resources/app'), '/path/cursor', 'not executable')
            (d / 'cursor').chmod(0o755)
            self.assertEqual(n.cli(cursor, app / 'resources/app'), str(d / 'cursor'))
            (d / 'cursor').unlink()
        state = {'windowsState': {'lastActiveWindow': {'folder': 'file:///c%3A/Users/x/My%20Repo'},
                                  'openedWindows': [{'folder': 'vscode-remote://wsl%2Bubuntu/srv/u/repo'}, {'folder': 'file:///d%3A/y'},
                                                    {'folder': 'vscode-remote://wsl+Debian' + quote(str(folder))},
                                                    {'folder': 'vscode-remote://wsl%2BUbuntu' + quote(str(folder))}, {}]}}
        appdata = self.tmp / 'appdata'
        (appdata / 'Code/User/globalStorage').mkdir(parents=True)
        (appdata / 'Code/User/globalStorage/storage.json').write_text(json.dumps(state))
        n.SYSTEM = 'win32'
        with unittest.mock.patch.dict(os.environ, {'APPDATA': str(appdata)}):
            got = n.folders('Code')
            self.assertEqual(got, [n.PureWindowsPath('c:/Users/x/My Repo'), n.PureWindowsPath('d:/y')])
            self.assertIn(got[0], n.PureWindowsPath('C:/USERS/x/my repo/sub').parents, 'Windows paths compare without case')
            self.assertEqual(n.folders('Cursor'), [])
        with unittest.mock.patch.dict(os.environ, {}):
            os.environ.pop('APPDATA', None)
            self.assertEqual(n.folders('Code'), [], 'no APPDATA')
        n.SYSTEM = 'wsl'
        roaming = n.WIN_USERS / 'alice/AppData/Roaming/Cursor [1]/User/globalStorage'
        roaming.mkdir(parents=True)
        (roaming / 'storage.json').write_text(json.dumps(state))
        (folder / 'sub').mkdir()
        with unittest.mock.patch.dict(os.environ, {'WSL_DISTRO_NAME': 'ubuntu'}):
            self.assertEqual(n.folders('Cursor [1]'), [n.PurePosixPath('/srv/u/repo'), n.PurePosixPath(folder)])
            self.assertEqual(n.window(str(folder / 'sub'), 'Cursor [1]'), n.PurePosixPath(folder))
            self.assertEqual(n.folders('Code'), [])
            ps = n.command('h', 'b', str(folder / 'sub'), {'nameShort': 'Cursor [1]', 'urlProtocol': 'cursor'}, None)
            self.assertIn(f'launch="cursor://vscode-remote/wsl+ubuntu{quote(str(folder))}"',
                          base64.b64decode(ps[4]).decode('utf-16-le'), "the window of the product's own nameShort")
        with unittest.mock.patch.dict(os.environ, {'WSL_DISTRO_NAME': ''}):
            self.assertEqual(n.folders('Cursor [1]'), [], 'no distribution name')
        unix = {'folder': 'file://' + quote(str(folder))}
        home, xdg = self.tmp / 'home', self.tmp / 'xdg'
        for base in (home / '.config/Code', xdg / 'Code', home / 'Library/Application Support/Code', xdg / 'None'):
            (base / 'User/globalStorage').mkdir(parents=True)
            (base / 'User/globalStorage/storage.json').write_text(json.dumps({'windowsState': {'openedWindows': [unix]}}))
            unix = {'folder': 'file:///other'}
        with unittest.mock.patch.object(n.Path, 'home', staticmethod(lambda: home)):
            for system, env, want in (('linux', {'XDG_CONFIG_HOME': str(xdg)}, '/other'), ('linux', {'XDG_CONFIG_HOME': ''}, str(folder)),
                                      ('linux', {}, str(folder)), ('darwin', {'XDG_CONFIG_HOME': str(xdg)}, '/other')):
                n.SYSTEM = system
                with unittest.mock.patch.dict(os.environ, env):
                    if not env:
                        os.environ.pop('XDG_CONFIG_HOME', None)
                    self.assertEqual(n.folders('Code'), [n.PurePosixPath(want)], (system, env))
            n.SYSTEM = 'linux'
            with unittest.mock.patch.dict(os.environ, {'XDG_CONFIG_HOME': str(xdg)}):
                for name in ('', None):
                    self.assertEqual(n.folders(name), [], 'no editor name')
            self.assertEqual(n.window(str(folder / 'sub'), None), n.PurePosixPath(folder / 'sub'), 'no name: git, else cwd')

    def notify_title(self, n):
        """title() reading backwards in chunks: the newest custom-title record at every chunk size, across chunk
        boundaries and long lines, never a quoted one."""
        def rec(name):
            return json.dumps({'type': 'custom-title', 'customTitle': name}).encode() + b'\n'
        quoted = json.dumps({'type': 'user', 'content': rec('Quoted').decode()}).encode() + b'\n'
        long = json.dumps({'type': 'assistant', 'x': 'z' * 300}).encode() + b'\n'
        t = self.tmp / 'chunks.jsonl'
        for data, want in ((rec('Old') + long + rec('New') + quoted + long, 'New'), (rec('Only') + long * 3, 'Only'),
                           (quoted + long, ''), (b'', ''), (rec('Last').rstrip(b'\n'), 'Last'), (long + rec('Ü é'), 'Ü é')):
            t.write_bytes(data)
            for chunk in (*range(1, 40), 64, 299, 300, 301, 1 << 20):
                n.CHUNK = chunk
                self.assertEqual(n.title(str(t)), want, (chunk, data[:60]))
        t.write_bytes(rec('Start') + json.dumps({'type': 'assistant', 'x': 'z' * (4 << 20)}).encode() + b'\n')
        n.CHUNK = 128
        start = time.monotonic()
        self.assertEqual(n.title(str(t)), 'Start')
        self.assertLess(time.monotonic() - start, 2, 'one 4 MiB line read in 128-byte chunks stays linear')
        n.CHUNK = 1 << 20
        # the search stops RESERVE seconds before the hook's deadline, leaving them to the fallback name and the notifier
        self.assertEqual(n.RESERVE, 3)
        t.write_bytes(rec('Present'))
        live = n.DEADLINE
        for deadline, want in ((time.monotonic() + 2.9, ''), (time.monotonic() - 100, ''), (time.monotonic() + 60, 'Present')):
            n.DEADLINE = deadline
            self.assertEqual(n.title(str(t)), want, deadline - time.monotonic())
        clock = iter(range(1000))  # one step per call: the loop tests the clock before each chunk
        n.time = types.SimpleNamespace(monotonic=lambda: next(clock))
        t.write_bytes(rec('Start') + json.dumps({'type': 'assistant', 'x': 'z' * 1000}).encode() + b'\n')
        n.CHUNK, n.DEADLINE = 64, n.RESERVE + 4.5
        self.assertEqual(n.title(str(t)), '', 'the deadline passes after five chunks, before the title')
        clock, n.DEADLINE = iter(range(1000)), n.RESERVE + 100.5
        self.assertEqual(n.title(str(t)), 'Start', 'enough chunks before the deadline')
        n.time, n.CHUNK, n.DEADLINE = time, 1 << 20, live

    def notify_workspace(self, n, folder):
        """storage(), local(), workspace() and folders() and window() on .code-workspace windows (SUP33)."""
        P, W, sub = n.PurePosixPath, n.PureWindowsPath, 'User/globalStorage/storage.json'

        @contextlib.contextmanager
        def system(name, **env):
            n.SYSTEM = name
            with unittest.mock.patch.dict(os.environ, {}), unittest.mock.patch.object(n.Path, 'home', staticmethod(lambda: self.tmp / 'home')):
                for k in ('VSCODE_PORTABLE', 'VSCODE_APPDATA', 'XDG_CONFIG_HOME', 'APPDATA', 'WSL_DISTRO_NAME'):
                    os.environ.pop(k, None)
                os.environ.update(env)
                yield
        home = self.tmp / 'home'
        for name, env, want in (('linux', {}, [home / '.config/Code' / sub]), ('linux', {'XDG_CONFIG_HOME': ''}, [home / '.config/Code' / sub]),
                                ('linux', {'XDG_CONFIG_HOME': '/x'}, [Path('/x/Code', sub)]),
                                ('linux', {'XDG_CONFIG_HOME': '/x', 'VSCODE_APPDATA': '/a'}, [Path('/a/Code', sub)]),
                                ('linux', {'XDG_CONFIG_HOME': '/x', 'VSCODE_APPDATA': '/a', 'VSCODE_PORTABLE': '/p'}, [Path('/p/user-data', sub)]),
                                ('linux', {'VSCODE_APPDATA': '/a', 'VSCODE_PORTABLE': ''}, [Path('/a/Code', sub)]),
                                ('darwin', {'XDG_CONFIG_HOME': '/x'}, [home / 'Library/Application Support/Code' / sub]),
                                ('darwin', {'VSCODE_APPDATA': '/a'}, [Path('/a/Code', sub)]), ('win32', {}, []),
                                ('win32', {'APPDATA': '/w'}, [Path('/w/Code', sub)]), ('win32', {'APPDATA': '/w', 'VSCODE_APPDATA': '/a'}, [Path('/a/Code', sub)]),
                                ('win32', {'VSCODE_PORTABLE': '/p'}, [Path('/p/user-data', sub)])):
            with system(name, **env):
                self.assertEqual(n.storage('Code'), want, (name, env))
                self.assertEqual((n.storage(''), n.storage(None)), ([], []), 'no editor name')
        for user in ('bob', 'alice'):
            (n.WIN_USERS / user / 'AppData/Roaming/Code/User/globalStorage').mkdir(parents=True)
            (n.WIN_USERS / user / 'AppData/Roaming/Code' / sub).write_text('{}')
        (n.WIN_USERS / 'carol/AppData/Roaming').mkdir(parents=True)
        with system('wsl', VSCODE_PORTABLE='/p', VSCODE_APPDATA='/a', APPDATA='/w'):
            self.assertEqual(n.storage('Code'), [n.WIN_USERS / u / 'AppData/Roaming/Code' / sub for u in ('alice', 'bob')],
                             "WSL: every Windows user's roaming folder, the portable and appdata variables ignored")
        for name, env, uri, want in (('linux', {}, 'file:///srv/x/My%20Repo', P('/srv/x/My Repo')), ('linux', {}, 'vscode-remote://wsl%2Bu/x', None),
                                     ('linux', {}, 'https://h/x', None), ('linux', {}, '', None),
                                     ('win32', {}, 'file:///c%3A/Users/x', W('c:/Users/x')), ('win32', {}, 'vscode-remote://wsl%2Bu/x', None),
                                     ('win32', {}, 'file://server/share/My%20Repo', W('//server/share/My Repo')),  # a UNC folder keeps its server
                                     ('wsl', {'WSL_DISTRO_NAME': 'Ubuntu'}, 'vscode-remote://wsl%2Bubuntu/srv/a%20b', P('/srv/a b')),
                                     ('wsl', {'WSL_DISTRO_NAME': 'Ubuntu'}, 'vscode-remote://wsl+Ubuntu/srv', P('/srv')),
                                     ('wsl', {'WSL_DISTRO_NAME': 'Ubuntu'}, 'vscode-remote://wsl%2BDebian/x', None),
                                     ('wsl', {'WSL_DISTRO_NAME': 'Ubuntu'}, 'file:///x', None), ('wsl', {}, 'vscode-remote://wsl%2Bubuntu/x', None)):
            with system(name, **env):
                got = n.local(uri)
                self.assertEqual((type(got), got), (type(want), want), (name, uri))
        d = self.tmp / 'ws dir'
        d.mkdir()
        ws = d / 'proj.code-workspace'
        ws.write_text(r'''// a workspace
{
  /* block
     comment */ "folders": [
    {"path": "sub"},  // relative
    {"path": "../other/./x"},
    {"path": "/abs/dir",},
    {"path": "back\\slash"},
    {"name": "a // b /* c */", "path": "named"},
    {"path": "t,}"},
    {"uri": "file:///c%3A/u/v%20w"},
    {"uri": "vscode-remote://wsl%2BUbuntu/srv/r"},
    {"uri": "vscode-remote://ssh-remote%2Bh/x"},
    {"name": "no path"},
    {"path": 5},
  ],
  "settings": {"url": "http://example.com//x",},
}
''')
        posix = [P(d / 'sub'), P(self.tmp / 'other/x'), P('/abs/dir'), P(d / 'back/slash'), P(d / 'named'), P(d / 't,}')]
        with system('linux'):
            self.assertEqual(n.workspace(P(ws)), posix + [P('/c:/u/v w')], 'comments, trailing commas, strings holding // and /* kept')
        with system('wsl', WSL_DISTRO_NAME='ubuntu'):
            self.assertEqual(n.workspace(P(ws)), posix + [P('/srv/r')], "WSL: this distribution's vscode-remote URIs")
        with system('win32'), unittest.mock.patch.object(n, 'Path', lambda p: ws if isinstance(p, W) else Path(p)):
            got = n.workspace(W('c:/ws dir/proj.code-workspace'))
            self.assertEqual(got, [W('c:/ws dir/sub'), W('c:/other/x'), W('c:/abs/dir'), W('c:/ws dir/back/slash'), W('c:/ws dir/named'),
                                   W('c:/ws dir/t,}'), W('c:/u/v w')])
            self.assertTrue(all(type(p) is W for p in got))
        uri = lambda p: 'file://' + quote(str(p))
        bad = {'missing': None, 'malformed': '{"folders": [', 'entry': '{"folders": [{"path": "ok"}, 5]}', 'utf8': b'\xff\xfe{',
               'list': '[1]', 'number': '{"folders": 5}'}
        for k, text in bad.items():
            if text is not None:
                (d / f'{k}.code-workspace').write_bytes(text if isinstance(text, bytes) else text.encode())
        deep = folder / 'deep.code-workspace'
        deep.write_text(json.dumps({'folders': [{'path': str(folder)}, {'path': 'sub'}]}))
        f1, f2 = self.tmp / 'f1', self.tmp / 'f2'
        windows = {'lastActiveWindow': {'workspaceIdentifier': {'id': 'w1', 'configURIPath': uri(ws)}},
                   'openedWindows': [{'folder': uri(f1)}, *({'workspaceIdentifier': {'configURIPath': uri(d / f'{k}.code-workspace')}} for k in bad),
                                     {'workspaceIdentifier': 'w2'}, {'workspaceIdentifier': {'configURIPath': 5}}, 5, {'folder': 7},
                                     {'workspaceIdentifier': {'configURIPath': 'vscode-remote://ssh-remote%2Bh/x.code-workspace'}},
                                     {'folder': uri(f2), 'workspaceIdentifier': {'configURIPath': uri(ws)}},
                                     {'workspaceIdentifier': {'configURIPath': uri(deep)}}]}
        xdg = self.tmp / 'xdg-ws'
        (xdg / 'Code/User/globalStorage').mkdir(parents=True)
        (xdg / 'Code' / sub).write_text(json.dumps({'windowsState': windows}))
        with system('linux', XDG_CONFIG_HOME=str(xdg)):
            self.assertEqual(n.folders('Code'), posix + [P('/c:/u/v w'), P(f1), P(f2), P(folder), P(folder / 'sub')],
                             "a workspace window's folders; a missing or malformed file or entry skips its window only; folder wins")
            (folder / 'sub/deeper').mkdir(parents=True, exist_ok=True)
            self.assertEqual(n.window(str(folder / 'sub/deeper'), 'Code'), P(folder / 'sub'), 'the deepest folder of a workspace window')
            self.assertEqual(n.window(str(folder), 'Code'), P(folder))
            self.assertEqual(n.window(str(d / 'sub'), 'Code'), P(d / 'sub'))
            f1.mkdir(exist_ok=True)
            self.assertEqual([n.window(str(c), 'Code', file=True) for c in (folder / 'sub/deeper', d / 'sub', f1)], [P(deep), P(ws), P(f1)],
                             "an opener gets a saved workspace window's file, which VS Code matches exactly, and a folder window's folder")

    def test_settings(self):
        hooks = json.loads((ROOT / '.claude/settings.json').read_text())['hooks']
        cmds = {ev: {(h.get('matcher'), x['command']) for h in hooks[ev] for x in h['hooks']} for ev in ('PreToolUse', 'PostToolUse', 'PostToolBatch')}
        self.assertIn(('Bash|Monitor', 'python3 "$CLAUDE_PROJECT_DIR/.claude/hooks/shell.py" pre'), cmds['PreToolUse'])
        self.assertIn(('Bash', 'python3 "$CLAUDE_PROJECT_DIR/.claude/hooks/shell.py" post'), cmds['PostToolUse'])
        self.assertIn(('Edit|Write|Read|NotebookEdit', 'python3 "$CLAUDE_PROJECT_DIR/.claude/hooks/guard.py"'), cmds['PreToolUse'])
        validator = 'python3 "$CLAUDE_PROJECT_DIR/.claude/hooks/consistency.py"'
        self.assertEqual(cmds['PostToolBatch'], {(None, validator)}, 'PostToolBatch takes no matcher')
        self.assertFalse(any(c == validator for ev in ('PreToolUse', 'PostToolUse') for _, c in cmds[ev]), 'one run per batch, not per edit')

    def enable_commit_hook(self):
        self.write('.claude/githooks/pre-commit', (ROOT / '.claude/githooks/pre-commit').read_text())
        (self.repo / '.claude/githooks/pre-commit').chmod(0o755)
        git(self.repo, 'add', '.claude/githooks')
        git(self.repo, 'commit', '-q', '-m', 'hook')
        git(self.repo, 'config', 'core.hooksPath', str(self.repo / '.claude/githooks'))

    def commit(self, *args, cwd=None):
        """(exit code, stderr, whether the validator ran) of `git commit -q --allow-empty -m c *args`."""
        self.log.unlink(missing_ok=True)
        r = subprocess.run(['git', 'commit', '-q', '--allow-empty', '-m', 'c', *args], cwd=cwd or self.repo, capture_output=True,
                           text=True, env=self.env, timeout=120)
        return r.returncode, r.stderr, self.log.exists()

    def stage(self, *ops, cwd=None):
        """Apply ops, stage every path they name and commit; (code, stderr, validator ran)."""
        for rel, text in ops:
            self.write(rel, text) if text is not None else (self.repo / rel).unlink()
            git(self.repo, 'add', '-A', '--', rel)
        return self.commit(cwd=cwd)

    def refused(self, got, *paths):
        code, err, _ = got
        self.assertEqual(code, 1, got)
        for p in paths:
            self.assertIn(p, err, got)

    def test_commit(self):
        self.enable_commit_hook()
        self.assertEqual(self.stage(('notes.txt', 'n2\n')), (0, '', False))
        self.refused(self.stage(('OLD/a.cpp', 'c\n')), 'Protected files: OLD/a.cpp', "git restore --staged -- OLD/a.cpp")
        git(self.repo, 'restore', '--staged', '--worktree', '--', 'OLD/a.cpp')
        self.refused(self.stage(('OLD/n.cpp', 'new\n')), 'OLD/n.cpp')
        git(self.repo, 'rm', '-q', '--cached', 'OLD/n.cpp')
        self.refused(self.stage(('OLD/a.cpp', None)), 'OLD/a.cpp')
        git(self.repo, 'restore', '--staged', '--worktree', '--', 'OLD/a.cpp')
        self.assertEqual(self.stage(('OLD/00-index.md', 'i2\n'))[:2], (0, ''))
        for rel in ('05-M/97-Legacy/x.cpp', '97-Legacy/t.cpp', '05-M/97-Legacy/n.cpp'):
            self.refused(self.stage((rel, 'edited\n')), rel)
            git(self.repo, 'reset', '-q', '--', rel)
        git(self.repo, 'checkout', '--', '05-M/97-Legacy/x.cpp', '97-Legacy/t.cpp')
        (self.repo / '05-M/97-Legacy/n.cpp').unlink()
        self.assertEqual(self.stage(('05-M/97-Legacy/x.cpp', None), ('05-M/97-Legacy/00-index.md', 'i\n'))[:2], (0, ''), 'excerpt deleted')
        self.refused(self.stage((TEMPLATE, 'hand\n')), TEMPLATE, '03-workspace.py')
        self.assertEqual(self.stage((TEMPLATE, 'generated\n'))[:2], (0, ''), 'regenerated over the staged hand edit')
        self.write(TEMPLATE, 'stale\n')
        git(self.repo, 'add', TEMPLATE)
        self.write(TEMPLATE, 'generated\n')
        self.refused(self.commit(), TEMPLATE)  # the staged copy is not the checked one
        git(self.repo, 'add', TEMPLATE)
        self.refused(self.stage((TEMPLATE, None)), TEMPLATE)
        git(self.repo, 'restore', '--staged', '--worktree', '--', TEMPLATE)
        git(self.repo, 'rm', '-q', '--cached', TEMPLATE)
        self.refused(self.commit(), TEMPLATE)  # deleted from the index while the working copy stays
        git(self.repo, 'reset', '-q', '--', TEMPLATE)
        for rel in ('05-M/00-index.md', '05-M/01-a.hpp', '00-Guidelines/13-Plan/packages.json', '08-Python/_01_a.py', 'docs/new.md'):
            self.assertEqual(self.stage((rel, 'v1\n')), (0, '', True), rel)
        for rel in ('tools/t.py', 'notes.txt', '05-M/x.cpp'):
            self.assertEqual(self.stage((rel, 'v2\n')), (0, '', False), rel)
        self.fail_file.write_text('Missing row: x\nBad link: y\n')
        got = self.stage(('05-M/00-index.md', 'v3\n'))
        self.refused(got, 'Consistency validator FAILED:\nMissing row: x\nBad link: y')
        self.assertTrue(got[2])
        git(self.repo, 'reset', '-q', '--', '05-M/00-index.md')
        self.assertEqual(self.stage(('notes.txt', 'v3\n')), (0, '', False), 'unwatched paths commit while the validator fails')
        self.fail_file.unlink()
        self.write('05-M/02-b.md', 'broken\n')
        git(self.repo, 'add', '05-M/02-b.md')
        self.write('05-M/02-b.md', 'fixed\n')  # fixed after a refusal but not staged again
        code, err, ran = self.commit()
        self.assertEqual((code, ran), (1, False), err)
        self.assertIn("Staged files differ from their working copies, which the validator reads: 05-M/02-b.md", err)
        self.assertIn('git add -A -- 05-M/02-b.md', err)
        git(self.repo, 'add', '05-M/02-b.md')
        self.assertEqual(self.commit(), (0, '', True))
        git(self.repo, 'rm', '-q', '--cached', '05-M/02-b.md')
        self.refused(self.commit(), 'working copies, which the validator reads: 05-M/02-b.md')
        (self.repo / '05-M/02-b.md').unlink()
        self.assertEqual(self.commit(), (0, '', True), 'deleted on disk too')
        self.write('OLD/a.cpp', 'unstaged\n')
        self.write('tools/t.py', 'partial\n')
        self.assertEqual(self.commit('--', 'tools/t.py'), (0, '', False), 'a partial commit checks its own paths')
        self.refused(self.commit('-a'), 'OLD/a.cpp')

    def test_commit_first(self):
        fresh = self.tmp / 'fresh'
        for rel, text in (('notes.txt', 'n\n'), ('.claude/githooks/pre-commit', (ROOT / '.claude/githooks/pre-commit').read_text())):
            (fresh / rel).parent.mkdir(parents=True, exist_ok=True)
            (fresh / rel).write_text(text)
        (fresh / '.claude/githooks/pre-commit').chmod(0o755)
        shutil.copytree(self.repo / '.claude/hooks', fresh / '.claude/hooks')
        git(fresh, 'init', '-q', '-b', 'master')
        git(fresh, 'config', 'core.hooksPath', str(fresh / '.claude/githooks'))
        for k, v in (('user.name', 'T'), ('user.email', 't@example.invalid')):
            git(fresh, 'config', k, v)
        git(fresh, 'add', 'notes.txt')
        self.assertEqual(self.commit(cwd=fresh), (0, '', False))
        (fresh / 'OLD').mkdir()
        (fresh / 'OLD/a.cpp').write_text('a\n')
        git(fresh, 'add', 'OLD/a.cpp')
        self.refused(self.commit(cwd=fresh), 'OLD/a.cpp')

    def test_commit_worktrees(self):
        self.enable_commit_hook()
        self.write('.gitignore', FILES['.gitignore'] + '.claude/worktrees/\n')
        git(self.repo, 'commit', '-q', '-am', 'ignore worktrees')
        self.fail_file.write_text('E\n')
        for name, trusted in (('DS07', True), ('PR7', False)):
            wt = self.repo / '.claude/worktrees' / name
            git(self.repo, 'worktree', 'add', '-q', '-b', name.lower(), str(wt))
            (wt / '.claude/githooks/pre-commit').write_text('#!/bin/sh\nexit 0\n')  # a worktree's own copy never runs
            (wt / '.claude/hooks/shell.py').write_text('import sys\nsys.exit(0)\n')
            git(wt, 'add', '.claude')
            self.assertEqual(self.commit(cwd=wt)[0], 0, name)
            (wt / '05-M/00-index.md').write_text('w\n')
            (wt / TEMPLATE).write_text('hand\n')
            git(wt, 'add', '05-M/00-index.md', TEMPLATE)
            code, err, ran = self.commit(cwd=wt)
            self.assertEqual((code, ran, TEMPLATE in err, 'Consistency validator FAILED:\nE' in err), (int(trusted), trusted, trusted, trusted), (name, err))
            (wt / 'OLD/a.cpp').write_text('w\n')
            git(wt, 'add', 'OLD/a.cpp')
            self.refused(self.commit(cwd=wt), 'OLD/a.cpp')

    def test_commit_error(self):
        outside = self.tmp / 'outside'
        outside.mkdir()
        r = subprocess.run([sys.executable, str(self.repo / '.claude/hooks/shell.py'), 'commit'], cwd=outside, capture_output=True,
                           text=True, env=self.env, timeout=60)
        self.assertEqual(r.returncode, 1, r)
        self.assertTrue(r.stderr.startswith('Commit refused: .claude/hooks/shell.py commit failed ('), r)

    def test_range(self):
        base = self.head()

        def check():
            r = subprocess.run([sys.executable, str(self.repo / '.claude/hooks/shell.py'), 'range', base], cwd=self.repo,
                               capture_output=True, text=True, env=self.env, timeout=60)
            return r.returncode, r.stderr
        self.assertEqual(check(), (0, ''))
        for rel, text in (('notes.txt', 'r\n'), ('OLD/00-index.md', 'r\n'), ('05-M/97-Legacy/00-index.md', 'r\n')):
            self.write(rel, text)
        git(self.repo, 'rm', '-q', '05-M/97-Legacy/x.cpp')
        git(self.repo, 'add', '-A')
        git(self.repo, 'commit', '-q', '-m', 'allowed')
        self.assertEqual(check(), (0, ''))
        self.write('OLD/a.cpp', 'r\n')
        self.write('97-Legacy/n.cpp', 'r\n')
        git(self.repo, 'add', '-A')
        git(self.repo, 'commit', '-q', '-m', 'forbidden')
        code, err = check()
        self.assertEqual(code, 1, err)
        self.assertIn(f'Protected files changed since {base[:12]}: 97-Legacy/n.cpp, OLD/a.cpp.', err)
        workflow = (ROOT / '.github/workflows/checks.yml').read_text()
        for step in ('python3 .claude/hooks/shell.py range "$BASE"', "python3 '97-Online Testing/03-workspace.py' --check",
                     "python3 '96-Local Testing/03-consistency.py'"):
            self.assertIn(step, workflow)

    def test_commit_random(self):
        rounds = 0 if MODE == 'quick' else 25 if MODE == 'full' else ROUNDS // 4
        paths = [p for p in FILES if p != '.gitignore'] + ['OLD/n.cpp', '05-M/97-Legacy/n.cpp', '05-M/02-b.md', 'tools/n.txt']
        for r in range(rounds):
            rng = random.Random(SEED * 1_000_033 + r)
            self.tearDown()
            self.setUp()
            self.enable_commit_hook()
            head = dict(FILES)
            index, work, history = dict(head), dict(head), []
            for c in range(6):
                for _ in range(rng.randint(1, 4)):
                    p, kind = rng.choice(paths), rng.choice(['write', 'write', 'delete', 'stage', 'stage'])
                    if kind == 'write':
                        work[p] = rng.choice(['a\n', 'generated\n', FILES.get(p, 'x\n')])
                        self.write(p, work[p])
                    elif kind == 'delete' and work.get(p) is not None:
                        (self.repo / p).unlink()
                        work[p] = None
                    elif kind == 'stage':
                        git(self.repo, 'add', '-A', '--', p) if work.get(p) is not None or index.get(p) is not None else None
                        index[p] = work.get(p)
                    history.append((kind, p))
                failing = rng.random() < 0.3
                self.fail_file.write_text('E\n') if failing else self.fail_file.unlink(missing_ok=True)
                staged = {p for p in set(head) | set(index) if head.get(p) != index.get(p)}
                bad = {p for p in staged if p != TEMPLATE and o_blocked(p, index.get(p), head)}
                template = TEMPLATE in staged and (index.get(TEMPLATE) != work.get(TEMPLATE) or work.get(TEMPLATE) != 'generated\n')
                watched = any(o_watched(p) for p in staged)
                stale = {p for p in staged if o_watched(p) and index.get(p) != work.get(p)}
                code, err, ran = self.commit()
                info = f'seed={SEED} mode={MODE} round={r} commit={c} failing={failing} history={history}'
                self.assertEqual(code, int(bool(bad or template or stale or (watched and failing))), f'{err}; {info}')
                self.assertEqual(ran, watched and not stale, info)
                for p in stale:
                    self.assertIn(p, err, info)
                for p in bad:
                    self.assertIn(p, err, info)
                if code == 0:
                    head = {p: t for p, t in index.items() if t is not None}
                    index = dict(head)
                    history.append(('commit', c))
                else:  # unstage everything, as the refusal leaves it to the committer
                    git(self.repo, 'reset', '-q')
                    index = dict(head)
                    history.append(('reset', c))

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
