#!/usr/bin/env python3
"""Checks for .claude/skills/setup-check/setup_check.py and the ISA helpers of 96-Local Testing/_00_cpp_test_runner.py.

No test touches the network or the real git configuration except the real-machine CLI runs of full mode. Fake
executables (`#!/bin/sh` scripts using only shell builtins, or `#!<this python>` scripts) sit in a per-test temporary
bin directory that is the whole PATH (plus a `git` symlink where the role is read); they print chosen versions for
`-dumpfullversion` and `-c <code>`, fail builds, build programs that exit nonzero or check their environment, and log
their arguments. The role comes from GIT_CONFIG_COUNT/KEY_0/VALUE_0 with GIT_CONFIG_GLOBAL and GIT_CONFIG_SYSTEM at
/dev/null (the real repository's local config may set compprog.role, so every real-repository role is overridden); the
unset case and the lint use temporary `git init` repositories. Probes are isolated by patching `out`, `module`,
`Path` and `platform.machine` of the loaded module.
quick: every fixed case, the deterministic CLI runs on a fake PATH and the real-repository lint. full: quick plus the
real-compiler probes, the real-machine CLI runs (detected distribution and `ubuntu`) and 2 seeded random rounds of
install (200 subsets x 3 distributions each), lint (one synthetic repository of 6-10 random files each) and CLI (one
random fake PATH each). stress: full with --rounds random rounds (default 6). Modes/seeds also come from
CP_TEST_MODE/CP_TEST_SEED.
Oracles (written here, independent of setup_check.py): a character scanner for version prefixes, for the `\\usepackage`
names of 98-Team Notebook/01-notebook.py and for os-release values; the `flags` lines of /proc/cpuinfo; a PATH scan for
presence; the install lines rebuilt per distribution from package kinds; the expected report of each lint template;
each CLI row rebuilt from the fake it faked. Failures print seed, mode, round and inputs.

Feature map:
TOOLS (non-empty fields, unique names, the required executable set, optional features, floors) -> test_tools_*;
version (the -dumpfullversion and -c PY_VERSION commands, major.minor prefix, unparsable) -> test_version_*;
check (floors of GCC 16, GCC 14 major 14, Python, PyPy; unknown version; not found; first present alternative; probe
problems joined and probe skipped below the floor) -> test_check_*;
probe (compiler build failure and nonzero run with flags and the 256 MB cap, real compilers; systemd-run exact
memory.max; gh auth exit code; site engine; fonts; TeX packages through kpsewhich; qemu without and with a cross
compiler and QEMU_LD_PREFIX; tools without a probe) -> test_probe_*;
platform_check (aarch64, AMD64, x86-64 with all flags, lacking bmi2, lacking several, no cpuinfo, this machine) ->
test_platform_*;
role_check (unset, contributor, maintainer, invalid, read in ROOT not the cwd) -> test_role_*;
distro (explicit choice, arch, ubuntu, ID_LIKE ubuntu/debian, ID_LIKE arch, fedora, unreadable) -> test_distro_*;
install (exact, empty, random subsets and distributions) -> test_install_*;
commands/undeclared (positive and negative call forms, basename of absolute names, OLD/ and 95-Resources/ skipped,
untracked and deleted skipped, modified read from the worktree, sorted output, random repositories, real repository and
--lint) -> test_commands_*, test_undeclared_*, test_lint_*;
CLI (rows, labels, WARN for optional and platform, Install lines for the distribution, exit code and last line, bad
distribution exits 2, no git on PATH, random fake PATHs, this machine) -> test_cli_*;
_triple/x86/x86_only/cpu_runs (triples, failing and missing compilers, cache per tuple, default $CXX split with shlex or
g++, flag copies, non-x86 compiler or machine, cpuinfo flags, this machine) -> test_isa_*.
"""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import platform
import random
import re
import shlex
import shutil
import string
import subprocess
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import Mock, patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()
import _00_cpp_test_runner as RUNNER  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
SETUP_PY = ROOT / '.claude/skills/setup-check/setup_check.py'
NOTEBOOK_PY = ROOT / '98-Team Notebook/01-notebook.py'
MODE, SEED, ROUNDS = 'full', 42, 6
FULL = {'test_probe_real_compilers', 'test_cli_real', 'test_cli_real_ubuntu', 'test_install_random', 'test_commands_random',
        'test_cli_random'}
REQUIRED = {'g++', 'g++-14', 'python3', 'git', 'rg', 'ccache', 'bwrap', 'systemd-run', 'lscpu', 'gh'}
FLOORS = {'GCC 16': (16,), 'GCC 14': (14,), 'Python': (3, 11), 'PyPy': (3, 10)}
PASS_LINE, FAIL_LINE = 'PASS: every required tool works', 'FAIL: a required tool is missing or below its floor'
SCOPE_HINT = 'enable a systemd user session with cgroup v2 and the memory controller delegated'
BOOM = 'fatal: boom'


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


S = load(SETUP_PY, 'setup_check_under_test')
TOOL = {t.name: t for t in S.TOOLS}
DECLARED = {e for t in S.TOOLS for e in t.exes}
ISOLATED = {k: v for k, v in os.environ.items() if not k.startswith(('GIT_', 'CLAUDE_CODE_AGENT', 'CLAUDE_EFFORT'))} | {
    'GIT_CONFIG_GLOBAL': os.devnull, 'GIT_CONFIG_SYSTEM': os.devnull, 'GIT_CONFIG_NOSYSTEM': '1'}
GIT = shutil.which('git', path=ISOLATED.get('PATH')) or '/usr/bin/git'
REAL_PATH = Path
REAL_RUN = subprocess.run
S.module('setup_plan', '00-Guidelines/13-Plan/plan.py')  # cached before any test moves ROOT
CAP = S.module('setup_memory_cap', '96-Local Testing/_00_memory_cap.py').available()  # cached under the real PATH


# Independent oracles

def o_version(text):
    """(major,) or (major, minor) from the leading digits of text, else None (character scan)."""
    i = 0
    while i < len(text) and text[i].isdigit():
        i += 1
    if not i:
        return None
    have, j = [int(text[:i])], i + 1
    if i < len(text) and text[i] == '.':
        while j < len(text) and text[j].isdigit():
            j += 1
        if j > i + 1:
            have.append(int(text[i + 1:j]))
    return tuple(have)


def o_usepackages(text):
    """Package names of every \\usepackage[...]{a,b} in text."""
    names, i, key = set(), 0, '\\usepackage'
    while (i := text.find(key, i)) >= 0:
        j = i + len(key)
        if text[j:j + 1] == '[':
            j = text.index(']', j) + 1
        if text[j:j + 1] == '{':
            k = text.index('}', j)
            names |= {n.strip() for n in text[j + 1:k].split(',')}
        i = j
    return names


def o_cpu_flags(text):
    flags = set()
    for line in text.splitlines():
        key, sep, rest = line.partition(':')
        if sep and key.strip() == 'flags':
            flags |= set(rest.split())
    return flags


def o_distro(text):
    """arch, ubuntu or None from the ID and ID_LIKE values of an os-release text."""
    words = []
    for line in text.splitlines():
        key, sep, value = line.partition('=')
        if sep and key in ('ID', 'ID_LIKE'):
            value = value.strip()
            if len(value) >= 2 and value[0] == value[-1] and value[0] in '"\'':
                value = value[1:-1]
            words += value.split()
    return 'arch' if 'arch' in words else 'ubuntu' if 'ubuntu' in words or 'debian' in words else None


def o_install(tools, where):
    lines = []
    for d, manager in (('ubuntu', 'sudo apt install'), ('arch', 'sudo pacman -S')):
        if where not in (None, d):
            continue
        kinds = [('aur' if p.startswith('AUR') else 'steps' if '&&' in p else 'plain', p) for p in (getattr(t, d) for t in tools)]
        plain = [p for k, p in kinds if k == 'plain']
        if plain:
            lines.append(f'  {d}: {manager} ' + ' '.join(plain))
        for k, p in kinds:
            if k == 'aur':
                lines.append(f'  {d}: {p}')
            elif k == 'steps':
                lines.append(f'  {d}: {manager} {p}')
    return lines


def o_which(exe, path):
    for d in path.split(os.pathsep):
        f = os.path.join(d or '.', exe)
        if os.path.isfile(f) and os.access(f, os.X_OK):
            return f
    return None


def o_row(name, need, ok, detail, label=None):
    label = label or ('OK' if ok else 'WARN' if need or name == 'platform' else 'FAIL')
    return label.ljust(4) + '  ' + name + (f' ({need})' if need else '') + ': ' + detail


def o_platform():
    cpu = REAL_PATH('/proc/cpuinfo')
    flags = o_cpu_flags(cpu.read_text()) if cpu.exists() else set()
    if platform.machine() not in ('x86_64', 'AMD64'):
        return False
    return {'avx2', 'fma', 'bmi2'} <= flags


def role_env(role):
    e = dict(ISOLATED)
    if role is not None:
        e |= {'GIT_CONFIG_COUNT': '1', 'GIT_CONFIG_KEY_0': 'compprog.role', 'GIT_CONFIG_VALUE_0': role}
    return e


def git(cwd, *args):
    r = REAL_RUN(['git', *args], cwd=cwd, capture_output=True, text=True, env=ISOLATED)
    if r.returncode:
        raise RuntimeError(f'git {" ".join(args)} failed in {cwd}: {r.stderr}')
    return r.stdout


def init_repo(path):
    path.mkdir(parents=True, exist_ok=True)
    git(path, 'init', '-q', '-b', 'master')
    git(path, 'config', 'user.name', 'Repo User')
    git(path, 'config', 'user.email', 'repo@example.invalid')
    return path


def executable(path, text):
    path.write_text(text)
    path.chmod(0o755)
    return path


def sh(path, body):
    return executable(path, '#!/bin/sh\n' + body)


def py(path, body):
    return executable(path, f'#!{sys.executable}\n' + body)


def fake_path(target, text=None, error=None):
    """Patch setup_check.Path so that Path(target) reads text (absent when None, raising error when given)."""
    class Fake:
        def __init__(self, p):
            self.p = p

        def exists(self):
            return text is not None and error is None

        def read_text(self, *a, **k):
            if error:
                raise error
            if text is None:
                raise FileNotFoundError(2, 'No such file or directory', target)
            return text

    return Mock(side_effect=lambda p, *rest: Fake(p) if str(p) == target and not rest else REAL_PATH(p, *rest))


COMPILER = r'''import json, os, sys
args = sys.argv[1:]
with open(os.environ.get('FAKE_LOG') or {log!r}, 'a') as f:
    f.write(json.dumps(args) + '\n')
mode = {mode!r}
if mode == 'fail':
    sys.stderr.write('note: first line\nfatal: boom\n')
    sys.exit(1)
if mode == 'quiet':
    sys.exit(1)
out = args[args.index('-o') + 1]
with open(out, 'w') as f:
    f.write({program!r})
os.chmod(out, 0o755)
'''
EXIT3 = '#!/bin/sh\necho first >&2\necho bad >&2\nexit 3\n'
LD_CHECK = '#!/bin/sh\n[ "$QEMU_LD_PREFIX" = /usr/aarch64-linux-gnu ] || { echo "prefix=$QEMU_LD_PREFIX" >&2; exit 5; }\n'
CAP_CHECK = ('#!/bin/sh\nm=$(cat "/sys/fs/cgroup$(sed -n "s/^0:://p" /proc/self/cgroup)/memory.max")\n'
             '[ "$m" = 268435456 ] || { echo "memory.max=$m" >&2; exit 4; }\n')


class Base(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix='setup-tester-'))
        self.addCleanup(lambda: shutil.rmtree(self.tmp, ignore_errors=True) if self._outcome.success else None)
        self.bin = self.tmp / 'bin'
        self.bin.mkdir()
        self.rng = random.Random(f'{SEED}:{self.id()}')

    def info(self, **kw):
        return f'seed={SEED} mode={MODE} fixture={self.tmp} ' + ' '.join(f'{k}={v!r}' for k, v in kw.items())

    def eq(self, actual, expected, **kw):
        self.assertEqual(actual, expected, self.info(**kw))

    def path_only(self, *dirs):
        return patch.dict(os.environ, {'PATH': os.pathsep.join(map(str, dirs or (self.bin,)))})

    def version_fake(self, name, output):
        log = self.tmp / f'log-{name}'
        sh(self.bin / name, f'printf "%s\\n" "$@" >> {shlex.quote(str(log))}\nprintf "%s\\n" {shlex.quote(output)}\n')
        return log

    def compiler(self, name, mode, program='#!/bin/sh\nexit 0\n'):
        log = self.tmp / f'log-{name}'
        return py(self.tmp / name, COMPILER.format(log=str(log), mode=mode, program=program)), log

    def rounds(self):
        return ROUNDS if MODE == 'stress' else 2


class Tools(Base):
    def test_tools_entries(self):
        for t in S.TOOLS:
            for field in ('name', 'ubuntu', 'arch'):
                self.assertTrue(isinstance(getattr(t, field), str) and getattr(t, field).strip(), self.info(tool=t, field=field))
            self.assertTrue(isinstance(t.exes, tuple) and t.exes and all(isinstance(e, str) and e for e in t.exes), self.info(tool=t))
        names = [t.name for t in S.TOOLS]
        self.eq(len(names), len(set(names)), names=names)

    def test_tools_required_set(self):
        self.eq({e for t in S.TOOLS if t.need is None for e in t.exes}, REQUIRED)

    def test_tools_optional_features(self):
        for t in S.TOOLS:
            if t.need is not None:
                self.assertTrue(isinstance(t.need, str) and t.need.strip(), self.info(tool=t))

    def test_tools_floors(self):
        self.eq({t.name: t.floor for t in S.TOOLS if t.floor}, FLOORS)


class Version(Base):
    def test_version_commands(self):
        log = self.version_fake('g++', '16.2.0')
        with self.path_only():
            self.eq(S.version(TOOL['GCC 16'], 'g++'), (16, 2))
        self.eq(log.read_text().splitlines(), ['-dumpfullversion'])
        log = self.version_fake('python3', '3.12.1')
        with self.path_only():
            self.eq(S.version(TOOL['Python'], 'python3'), (3, 12))
        args = log.read_text().splitlines()
        self.eq(args[0], '-c', args=args)
        real = REAL_RUN([sys.executable, '-c', args[1]], capture_output=True, text=True).stdout.strip()
        self.eq(real, '.'.join(map(str, sys.version_info[:3])), code=args[1])

    def test_version_parse(self):
        for text in ('16.0.1', '16', '15.99', '3.10.14', '14.3.0-ubuntu', 'gcc (GCC) 16.2.0', '', 'x.1', '16.', '7.a'):
            self.version_fake('g++', text)
            with self.path_only():
                self.eq(S.version(TOOL['GCC 16'], 'g++'), o_version(text), output=text)

    def check(self, tool, exe, text, ok, detail):
        self.version_fake(exe, text)
        with self.path_only(), patch.object(S, 'probe', return_value=[]) as probe:
            self.eq(S.check(TOOL[tool]), (ok, detail), tool=tool, output=text)
        self.eq(probe.called, ok, tool=tool, output=text, what='probe runs only above the floor')

    def test_check_gcc16(self):
        self.check('GCC 16', 'g++', '15.2.0', False, 'g++ 15.2, needs 16')
        self.check('GCC 16', 'g++', '16.0.1', True, 'g++ 16.0')
        self.check('GCC 16', 'g++', '16', True, 'g++ 16')
        self.check('GCC 16', 'g++', '17.1.0', True, 'g++ 17.1')

    def test_check_gcc14(self):
        self.check('GCC 14', 'g++-14', '14.3.0', True, 'g++-14 14.3')
        self.check('GCC 14', 'g++-14', '15.1.0', False, 'g++-14 15.1, needs 14')
        self.check('GCC 14', 'g++-14', '13.2', False, 'g++-14 13.2, needs 14')

    def test_check_python(self):
        self.check('Python', 'python3', '3.10.14', False, 'python3 3.10, needs 3.11')
        self.check('Python', 'python3', '3.11.0', True, 'python3 3.11')
        self.check('Python', 'python3', '4.0', True, 'python3 4.0')
        self.check('Python', 'python3', '2.7.18', False, 'python3 2.7, needs 3.11')

    def test_check_pypy(self):
        self.check('PyPy', 'pypy3', '3.9.18', False, 'pypy3 3.9, needs 3.10')
        self.check('PyPy', 'pypy3', '3.10.14', True, 'pypy3 3.10')

    def test_check_unknown_version(self):
        self.check('GCC 16', 'g++', 'gcc version sixteen', False, 'g++ unknown version, needs 16')
        self.check('Python', 'python3', '', False, 'python3 unknown version, needs 3.11')

    def test_check_missing(self):
        with self.path_only(), patch.object(S, 'probe', return_value=[]) as probe:
            for t in S.TOOLS:
                self.eq(S.check(t), (False, 'not found'), tool=t.name)
        self.assertFalse(probe.called, self.info())

    def test_check_alternatives(self):
        node = TOOL['Node.js']
        self.eq(node.exes, ('node', 'nodejs'))
        sh(self.bin / 'nodejs', 'exit 0\n')
        with self.path_only(), patch.object(S, 'probe', return_value=[]) as probe:
            self.eq(S.check(node), (True, 'nodejs'), present=['nodejs'])
            probe.assert_called_with(node, 'nodejs')
            sh(self.bin / 'node', 'exit 0\n')
            self.eq(S.check(node), (True, 'node'), present=['node', 'nodejs'])
            probe.assert_called_with(node, 'node')

    def test_check_probe_problems(self):
        self.version_fake('g++', '16.1.0')
        sh(self.bin / 'git', 'exit 0\n')
        with self.path_only(), patch.object(S, 'probe', return_value=['p one', 'p two']) as probe:
            self.eq(S.check(TOOL['GCC 16']), (False, 'p one; p two'))
            probe.assert_called_with(TOOL['GCC 16'], 'g++')
            self.eq(S.check(TOOL['git']), (False, 'p one; p two'))


class Probe(Base):
    def compiler_probe(self, mode, program, expected):
        exe, log = self.compiler('fake-gxx', mode, program)
        for tool in (TOOL['GCC 16'], TOOL['GCC 14']):
            log.unlink(missing_ok=True)
            self.eq(S.probe(tool, str(exe)), [f'ASan/UBSan {expected}', f'static {expected}'], tool=tool.name, mode_=mode)
            calls = [json.loads(line) for line in log.read_text().splitlines()]
            self.eq(len(calls), 2, calls=calls)
            for call, flag in zip(calls, ('-fsanitize=address,undefined', '-static')):
                self.assertTrue('-std=gnu++20' in call and flag in call and '-o' in call and call[-2] == '-o', self.info(call=call))

    def test_probe_compiler_build_fails(self):
        self.compiler_probe('fail', '', 'build: ' + BOOM)
        self.compiler_probe('quiet', '', 'build: failed')

    def test_probe_compiler_run_fails(self):
        self.compiler_probe('ok', EXIT3, 'run exit 3: bad')

    def test_probe_compiler_passes_and_capped(self):
        exe, _ = self.compiler('fake-gxx', 'ok', CAP_CHECK if CAP else '#!/bin/sh\nexit 0\n')
        self.eq(S.probe(TOOL['GCC 16'], str(exe)), [], capped=CAP)
        if not CAP:
            self.skipTest('SKIP: no memory-capped user scope here; the 256 MB cap of probe runs is untested')

    def test_probe_real_compilers(self):
        found = 0
        for tool in (TOOL['GCC 16'], TOOL['GCC 14']):
            exe = o_which(tool.exes[0], os.environ.get('PATH', ''))
            if exe:
                found += 1
                self.eq(S.probe(tool, tool.exes[0]), [], tool=tool.name, exe=exe)
        if not found:
            self.skipTest('SKIP: neither g++ nor g++-14 is installed')

    def test_probe_systemd(self):
        tool = TOOL['systemd user manager']
        for result, expected in (((0, str(64 * 1024 * 1024)), []), ((0, str(128 * 1024 * 1024)), ['134217728']), ((0, 'max'), ['max']),
                                 ((None, 'no such file'), ['no such file']), ((1, ''), ['1'])):
            with patch.object(S, 'out', return_value=result) as out:
                want = [f'no memory-capped user scope ({e}): {SCOPE_HINT}' for e in expected]
                self.eq(S.probe(tool, 'systemd-run'), want, out=result)
            cmd = out.call_args[0][0]
            self.assertTrue(cmd[:3] == ['systemd-run', '--user', '--scope'] and 'MemoryMax=64M' in cmd, self.info(cmd=cmd))

    def test_probe_gh(self):
        for result, want in (((0, 'Logged in'), []), ((1, 'not logged'), ['not logged in: gh auth login']),
                             ((None, 'gone'), ['not logged in: gh auth login'])):
            with patch.object(S, 'out', return_value=result) as out:
                self.eq(S.probe(TOOL['GitHub CLI'], 'gh'), want, out=result)
            self.eq(out.call_args[0][0], ['gh', 'auth', 'status'])

    def test_probe_site(self):
        for found, want in ((('zensical', '/x/zensical'), []), ('zensical not found; mkdocs not found', ['zensical not found; mkdocs not found'])):
            site = SimpleNamespace(engine=lambda: found)
            with patch.object(S, 'module', return_value=site) as module:
                self.eq(S.probe(TOOL['site engine'], 'mkdocs'), want, engine=found)
            self.eq(module.call_args[0], ('setup_site', '98-Team Notebook/06-site.py'))

    def test_probe_fonts(self):
        cfg = {'body_font': 'Body Sans', 'code_font': 'Code Mono'}
        for status, want in (({'Body Sans': True, 'Code Mono': True}, []), ({'Body Sans': False, 'Code Mono': True}, ["font 'Body Sans' missing"]),
                             ({'Body Sans': False, 'Code Mono': False}, ["font 'Body Sans' missing", "font 'Code Mono' missing"]),
                             ({'Body Sans': None, 'Code Mono': None}, [])):
            book = SimpleNamespace(sources=lambda: {'a': 1}, load=lambda available: cfg if available == {'a': 1} else {},
                                   font_status=lambda f, status=status: status[f])
            with patch.object(S, 'module', return_value=book) as module:
                self.eq(S.probe(TOOL['fontconfig'], 'fc-list'), want, status=status)
            self.eq(module.call_args[0], ('setup_notebook', '98-Team Notebook/01-notebook.py'))

    def test_probe_lualatex(self):
        names = sorted(o_usepackages(NOTEBOOK_PY.read_text()))
        self.assertIn('fontspec', names, self.info(names=names, what='oracle sanity'))
        log = self.tmp / 'log-kpsewhich'
        for missing in ([], [self.rng.choice(names)], sorted(self.rng.sample(names, 2))):
            log.unlink(missing_ok=True)
            cases = ''.join(f'  {shlex.quote(n + ".sty")}) exit 1;;\n' for n in missing)
            sh(self.bin / 'kpsewhich', f'printf "%s\\n" "$1" >> {shlex.quote(str(log))}\ncase "$1" in\n{cases}esac\necho "/tex/$1"\n')
            with self.path_only():
                self.eq(S.probe(TOOL['LuaLaTeX'], 'lualatex'), [f'TeX package {n} missing' for n in missing], missing=missing)
            self.eq(sorted(log.read_text().split()), sorted(n + '.sty' for n in names), what='kpsewhich queries')
        (self.bin / 'kpsewhich').unlink()
        with self.path_only():
            self.eq(S.probe(TOOL['LuaLaTeX'], 'lualatex'), [], what='no kpsewhich')

    def test_probe_qemu_missing_cross(self):
        with self.path_only():
            problems = S.probe(TOOL['qemu-user'], 'qemu-aarch64')
        self.eq(problems, ['needs aarch64-linux-gnu-g++ to build its arm64 probe'])

    def test_probe_qemu_cross(self):
        for mode, program, want in (('ok', LD_CHECK, []), ('ok', EXIT3, ['run exit 3: bad']), ('fail', '', ['build: ' + BOOM])):
            exe, _ = self.compiler('aarch64-linux-gnu-g++', mode, program)
            shutil.move(exe, self.bin / exe.name)
            with self.path_only(self.bin, *os.environ.get('PATH', '').split(os.pathsep)):
                self.eq(S.probe(TOOL['qemu-user'], 'qemu-aarch64'),
                        [f'arm64 binary does not run directly ({w}): register binfmt (qemu-user-binfmt)' for w in want], mode_=mode, program=program)

    def test_probe_other_tools_none(self):
        with patch.object(S, 'out', side_effect=AssertionError('no command')), patch.object(S, 'module', side_effect=AssertionError('no module')):
            for name in ('git', 'ripgrep', 'ccache', 'bubblewrap', 'lscpu', 'kpathsea', 'Node.js', 'oj-bundle', 'PyPy', 'arm64 cross g++'):
                t = TOOL[name]
                self.eq(S.probe(t, t.exes[0]), [], tool=name)


class Platform(Base):
    CPU = 'processor\t: 0\nmodel name\t: Fake CPU\nflags\t\t: fpu sse2 {flags}\nbugs\t\t: none\n\nprocessor\t: 1\nflags\t\t: fpu sse2 {flags}\n'

    def platform(self, machine, cpuinfo):
        fake = fake_path('/proc/cpuinfo', cpuinfo)
        with patch.object(S.platform, 'machine', return_value=machine), patch.object(S, 'Path', fake):
            return S.platform_check()

    def test_platform_aarch64(self):
        ok, detail = self.platform('aarch64', self.CPU.format(flags='avx2 fma bmi2'))
        self.assertTrue(not ok and 'x86-64' in detail and detail.startswith('aarch64'), self.info(detail=detail))

    def test_platform_x86_all(self):
        for machine in ('x86_64', 'AMD64'):
            self.eq(self.platform(machine, self.CPU.format(flags='bmi1 avx2 fma bmi2')), (True, 'x86-64 with AVX2, FMA and BMI2'), machine=machine)

    def test_platform_x86_lacks_bmi2(self):
        ok, detail = self.platform('x86_64', self.CPU.format(flags='avx2 fma bmi1'))
        self.eq((ok, detail), (False, 'x86-64 lacking bmi2'))

    def test_platform_x86_lacks_several(self):
        self.eq(self.platform('x86_64', self.CPU.format(flags='avx2')), (False, 'x86-64 lacking bmi2, fma'))
        self.eq(self.platform('x86_64', None), (False, 'x86-64 lacking avx2, bmi2, fma'), cpuinfo=None)

    def test_platform_real(self):
        ok, _ = S.platform_check()
        self.eq(ok, o_platform(), machine=platform.machine())


class Role(Base):
    def role(self, role, root=None):
        with patch.dict(os.environ, role_env(role), clear=True), patch.object(S, 'ROOT', root or S.ROOT):
            return S.role_check()

    def test_role_unset(self):
        repo = init_repo(self.tmp / 'repo')
        self.eq(self.role(None, repo), (True, 'contributor'))

    def test_role_overrides(self):
        for role in ('contributor', 'maintainer'):
            self.eq(self.role(role), (True, role), role=role)
        self.eq(self.role(''), (True, 'contributor'), role='')

    def test_role_without_git(self):
        with patch.object(S.shutil, 'which', lambda e, *a, **k: None if e == 'git' else shutil.which(e, *a, **k)):
            self.eq(S.role_check(), (False, 'needs git'))

    def test_role_invalid(self):
        self.eq(self.role('admin'), (False, "git config compprog.role is 'admin'; set contributor or maintainer"))

    def test_role_read_in_root(self):
        a, b = init_repo(self.tmp / 'a'), init_repo(self.tmp / 'b')
        git(a, 'config', 'compprog.role', 'maintainer')
        git(b, 'config', 'compprog.role', 'admin')
        cwd = os.getcwd()
        os.chdir(b)
        try:
            self.eq(self.role(None, a), (True, 'maintainer'), root=a, cwd=b)
        finally:
            os.chdir(cwd)


class Distro(Base):
    def distro(self, choice, text=None, error=None):
        with patch.object(S, 'Path', fake_path('/etc/os-release', text, error)):
            return S.distro(choice)

    def test_distro_explicit(self):
        for choice in ('ubuntu', 'arch'):
            self.eq(self.distro(choice, error=PermissionError(13, 'denied')), choice, choice=choice)

    def test_distro_os_release(self):
        cases = [('NAME="Arch Linux"\nPRETTY_NAME="Arch Linux"\nID=arch\nBUILD_ID=rolling\n', 'arch'),
                 ('NAME="Ubuntu"\nVERSION_ID="26.04"\nID=ubuntu\nID_LIKE=debian\n', 'ubuntu'),
                 ('ID="ubuntu"\n', 'ubuntu'),
                 ("ID='arch'\n", 'arch'),
                 ('NAME="Linux Mint"\nID=linuxmint\nID_LIKE="ubuntu debian"\n', 'ubuntu'),
                 ('ID=debian\n', 'ubuntu'),
                 ('NAME="Manjaro Linux"\nID=manjaro\nID_LIKE=arch\n', 'arch'),
                 ('NAME="Fedora Linux"\nID=fedora\nVERSION_ID=42\n', None),
                 ('VERSION_ID=arch\nNAME=ubuntu\nID=fedora\n', None),
                 ('', None)]
        for text, want in cases:
            self.eq(self.distro('', text), want, os_release=text)
            self.eq(o_distro(text), want, os_release=text, what='oracle sanity')

    def test_distro_unreadable(self):
        for error in (FileNotFoundError(2, 'missing'), PermissionError(13, 'denied'), IsADirectoryError(21, 'dir')):
            self.eq(self.distro('', error=error), None, error=error)


class Install(Base):
    def test_install_exact(self):
        tools = [TOOL['GCC 16'], TOOL['GCC 14'], TOOL['git'], TOOL['site engine']]
        want = ['  ubuntu: sudo apt install g++-14 git',
                '  ubuntu: sudo apt install g++-16 && mkdir -p ~/.local/bin && ln -sf /usr/bin/g++-16 ~/.local/bin/g++',
                '  ubuntu: sudo apt install pipx && pipx install zensical',
                '  arch: sudo pacman -S gcc git',
                '  arch: AUR: gcc14 gcc14-libs',
                '  arch: sudo pacman -S python-pipx && pipx install zensical']
        self.eq(S.install(tools, None), want)
        self.eq(S.install(tools, 'ubuntu'), want[:3])
        self.eq(S.install(tools, 'arch'), want[3:])
        self.eq(S.install([TOOL['GCC 16']], 'ubuntu'), [want[1]], what='no plain line without plain packages')
        self.eq(S.install([TOOL['GCC 14']], 'arch'), ['  arch: AUR: gcc14 gcc14-libs'])

    def test_install_empty(self):
        for where in ('ubuntu', 'arch', None):
            self.eq(S.install([], where), [], where=where)

    def test_install_random(self):
        for r in range(self.rounds()):
            for _ in range(200):
                tools = [t for t in S.TOOLS if self.rng.random() < self.rng.random()]
                for where in ('ubuntu', 'arch', None):
                    self.eq(S.install(tools, where), o_install(tools, where), round=r, where=where, tools=[t.name for t in tools])


POSITIVE = ["subprocess.run(['X', '-v'], check=True)", "subprocess.Popen(('X',))", "subprocess.check_output([os.environ.get('CXX', 'X'), '-c'])",
            "subprocess.call(['X'])", "subprocess.check_call(('X', 'a', 'b'))", "shutil.which('X')", "which('X')",
            "subprocess.run(args=['X'])", "sp.run(['X'])", "check_output(['X'])", "w('X')"]
NEGATIVE = ["self.run(['X'])", "subprocess.run(cmd)", "subprocess.run([var, 'X'])", "subprocess.run(f'X {flag}', shell=True)",
            "shutil.which(name)", "os.system('X')", "subprocess.run([])", "subprocess.getoutput('X')",
            "subprocess.check_output([os.environ.get('CXX'), 'X'])", "subprocess.run(['X'] + rest)", "runner.run(['X'])"]
HEADER = ('import os\nimport shutil\nimport subprocess\nimport subprocess as sp\nfrom shutil import which\n'
          'from shutil import which as w\nfrom subprocess import check_output\n')


def wrap(lines, rng):
    """A module text holding lines at top level, in a function or in a method (each parsed by ast.walk)."""
    out = [HEADER]
    for line in lines:
        out.append(rng.choice([f'{line}\n', f'def f():\n    {line}\n', f'class C:\n    def m(self):\n        if x:\n            {line}\n']))
    return ''.join(out)


class Lint(Base):
    def test_commands_forms(self):
        text = HEADER + '''
def main(cmd, var, name):
    subprocess.run(['x', '-q'])
    subprocess.Popen(('y',))
    subprocess.check_output([os.environ.get('CXX', 'zz'), '-c'])
    shutil.which('w')
    subprocess.run(['/usr/bin/v', 'arg'])
    self.run(['n1'])
    subprocess.run(cmd)
    subprocess.run([var, 'n3'])
    subprocess.run(f'n4 {var}', shell=True)
    which(name)
    shutil.which(name)
    os.system('n2')
    which('wh')
    sp.run(args=['al'])
    check_output(['co'])
    w('ww')
    subprocess.run([os.getenv('CC', 'ge'), '-v'])
'''
        f = self.tmp / 'forms.py'
        f.write_text(text)
        self.eq(S.commands(f), {'x', 'y', 'zz', 'w', '/usr/bin/v', 'wh', 'al', 'co', 'ww', 'ge'}, text=text)

    def test_undeclared_synthetic(self):
        repo = init_repo(self.tmp / 'repo')
        files = {'a.py': ['git', 'undecl1'], 'sub/dir/b.py': ['/usr/bin/v', '/usr/bin/git', 'g++'], 'OLD/c.py': ['old1'],
                 'OLDER/c.py': ['older1'], '95-Resources/r.py': ['res1'], 'z z.py': ['spaced'], 'notes.txt': ['txt1'],
                 'gone.py': ['gone1'], 'm.py': ['before1']}
        for f, names in files.items():
            (repo / f).parent.mkdir(parents=True, exist_ok=True)
            (repo / f).write_text(HEADER + ''.join(f"subprocess.run(['{n}'])\n" for n in names))
        git(repo, 'add', '-A')
        git(repo, 'commit', '-q', '-m', 'init')
        (repo / 'u.py').write_text(HEADER + "shutil.which('untracked1')\n")
        (repo / 'gone.py').unlink()
        (repo / 'm.py').write_text(HEADER + "shutil.which('after1')\n")
        want = [('OLDER/c.py', 'older1'), ('a.py', 'undecl1'), ('m.py', 'after1'), ('sub/dir/b.py', '/usr/bin/v'), ('z z.py', 'spaced')]
        with patch.dict(os.environ, ISOLATED, clear=True):
            self.eq(S.undeclared(repo), want, files=files)

    def test_commands_random(self):
        pool = [f'tool{c}' for c in string.ascii_lowercase[:8]] + ['/opt/q/toolz', '/usr/bin/git', 'git', 'g++-14', 'node', 'qemu-aarch64']
        for r in range(self.rounds()):
            repo = init_repo(self.tmp / f'repo{r}')
            specs, want = [], []
            for i in range(self.rng.randint(6, 10)):
                folder = self.rng.choice(['', 'a/', 'b/c/', 'OLD/', 'OLDER/', '95-Resources/', 'old/'])
                name = folder + self.rng.choice(['s', 'my tool ', 'x-']) + f'{i}.py'
                status = self.rng.choice(['committed', 'staged', 'modified', 'untracked', 'deleted'])
                lines = [(self.rng.choice(POSITIVE if pos else NEGATIVE).replace('X', n), n if pos else None)
                         for n, pos in ((self.rng.choice(pool), self.rng.random() < 0.6) for _ in range(self.rng.randint(0, 5)))]
                before = wrap([self.rng.choice(POSITIVE).replace('X', 'early1')], self.rng)
                text = wrap([line for line, _ in lines], self.rng)
                specs.append((name, status, lines))
                (repo / name).parent.mkdir(parents=True, exist_ok=True)
                (repo / name).write_text(before if status == 'modified' else text)
                expect = {n for _, n in lines if n}
                if status != 'modified':
                    self.eq(S.commands(repo / name), expect, round=r, file=name, text=text)
                if status in ('committed', 'staged', 'modified') and not name.startswith(('OLD/', '95-Resources/')):
                    want += [(name, n) for n in expect if n.rsplit('/', 1)[-1] not in DECLARED]
            committed = [n for n, s, _ in specs if s in ('committed', 'modified', 'deleted')]
            if committed:
                git(repo, 'add', '--', *committed)
                git(repo, 'commit', '-q', '-m', 'init')
            staged = [n for n, s, _ in specs if s == 'staged']
            if staged:
                git(repo, 'add', '--', *staged)
            for name, status, lines in specs:
                if status == 'deleted':
                    (repo / name).unlink()
                elif status == 'modified':
                    (repo / name).write_text(wrap([line for line, _ in lines], self.rng))
                    self.eq(S.commands(repo / name), {n for _, n in lines if n}, round=r, file=name)
            with patch.dict(os.environ, ISOLATED, clear=True):
                self.eq(S.undeclared(repo), sorted(want), round=r, files=specs)

    def test_undeclared_real(self):
        self.eq(S.undeclared(), [], root=S.ROOT)

    def test_undeclared_not_a_repo(self):
        (self.tmp / 'plain').mkdir()
        (self.tmp / 'plain/a.py').write_text("import shutil\nshutil.which('undeclaredtool')\n")
        with self.assertRaises(subprocess.CalledProcessError, msg=self.info(root=self.tmp / 'plain')):
            S.undeclared(self.tmp / 'plain')

    def test_lint_cli_real(self):
        r = REAL_RUN([sys.executable, str(SETUP_PY), '--lint'], capture_output=True, text=True, timeout=300)
        self.eq((r.returncode, r.stdout, r.stderr), (0, '', ''), cmd='setup_check.py --lint')


class Cli(Base):
    def run_cli(self, *args, env=None):
        return REAL_RUN([sys.executable, str(SETUP_PY), *args], capture_output=True, text=True, timeout=600, env=env)

    def fake_bin(self, spec, kpse_missing=()):
        """Fakes on self.bin per spec {exe: value} plus a git symlink; returns the per-tool expected (ok, detail)."""
        for exe, value in spec.items():
            f = self.bin / exe
            if exe in ('g++', 'g++-14', 'aarch64-linux-gnu-g++'):
                sh(f, f'case "$1" in -dumpfullversion) printf "%s\\n" {shlex.quote(value or "")}; exit 0;; esac\necho {shlex.quote(BOOM)} >&2\nexit 1\n')
            elif exe in ('python3', 'pypy3'):
                sh(f, f'printf "%s\\n" {shlex.quote(value)}\n')
            elif exe == 'gh':
                sh(f, f'[ "$1 $2" = "auth status" ] && exit {value}\nexit 9\n')
            elif exe == 'systemd-run':
                sh(f, f'printf "%s\\n" {shlex.quote(value)}\n')
            elif exe == 'kpsewhich':
                cases = ''.join(f'  {shlex.quote(n + ".sty")}) exit 1;;\n' for n in kpse_missing)
                sh(f, f'case "$1" in\n{cases}esac\necho "/tex/$1"\n')
            else:
                sh(f, 'exit 0\n')
        (self.bin / 'git').symlink_to(GIT)
        rows = {}
        for t in S.TOOLS:
            exe = 'git' if t.name == 'git' else next((e for e in t.exes if e in spec), None)
            if exe is None:
                rows[t.name] = (False, 'not found')
                continue
            have = o_version(spec[exe]) if t.floor else None
            floor = '.'.join(map(str, t.floor or ()))
            if t.floor and (have is None or have[:len(t.floor)] < t.floor or (t.name == 'GCC 14' and have[0] != 14)):
                rows[t.name] = (False, f"{exe} {'.'.join(map(str, have)) if have else 'unknown version'}, needs {floor}")
                continue
            problems = []
            if exe in ('g++', 'g++-14'):
                problems = [f'ASan/UBSan build: {BOOM}', f'static build: {BOOM}']
            elif exe == 'systemd-run' and spec[exe] != '67108864':
                problems = [f'no memory-capped user scope ({spec[exe] or 0}): {SCOPE_HINT}']
            elif exe == 'gh' and spec[exe] != '0':
                problems = ['not logged in: gh auth login']
            elif exe == 'lualatex' and 'kpsewhich' in spec:
                problems = [f'TeX package {n} missing' for n in sorted(kpse_missing)]
            elif exe == 'qemu-aarch64':
                problems = [f'arm64 binary does not run directly (build: {BOOM}): register binfmt (qemu-user-binfmt)'
                            if 'aarch64-linux-gnu-g++' in spec else 'needs aarch64-linux-gnu-g++ to build its arm64 probe']
            rows[t.name] = (not problems, '; '.join(problems) or exe + (' ' + '.'.join(map(str, have)) if have else ''))
        return rows

    def expected(self, rows, role, where):
        ok_platform = o_platform()
        role_row = (True, role or 'contributor') if role in (None, '', 'contributor', 'maintainer') else \
            (False, f'git config compprog.role is {role!r}; set contributor or maintainer')
        lines = [o_row(t.name, t.need, *rows[t.name]) for t in S.TOOLS] + [o_row('role', None, *role_row)]
        # a failed gh or systemd-run probe (login, delegation) is configuration: install lines only when absent
        missing = [t for t in S.TOOLS if not rows[t.name][0] and (rows[t.name][1] == 'not found' or t.exes[0] not in ('gh', 'systemd-run'))]
        if missing:
            lines += ['Install:', *o_install(missing, where)]
        failed = not role_row[0] or any(not rows[t.name][0] and t.need is None for t in S.TOOLS)
        return ok_platform, lines + [FAIL_LINE if failed else PASS_LINE], int(failed)

    def check_cli(self, spec, role, choice, kpse_missing=(), **kw):
        rows = self.fake_bin(spec, kpse_missing)
        env = role_env(role) | {'PATH': str(self.bin)}
        where = choice or o_distro(REAL_PATH('/etc/os-release').read_text())
        ok_platform, want, code = self.expected(rows, role, where)
        r = self.run_cli(*([choice] if choice else []), env=env)
        info = dict(spec=spec, role=role, distro=choice, kpse_missing=kpse_missing, **kw)
        lines = r.stdout.splitlines()
        self.eq((r.returncode, r.stderr), (code, ''), stdout=r.stdout, **info)
        self.assertTrue(lines and lines[0].startswith(('OK    platform: ' if ok_platform else 'WARN  platform: ')), self.info(stdout=r.stdout, **info))
        self.eq(lines[1:], want, **info)

    def test_cli_fake_path(self):
        spec = {'g++': '15.2.0', 'python3': '3.11.0', 'rg': '', 'ccache': '', 'bwrap': '', 'lscpu': '', 'gh': '0', 'node': ''}
        for choice in ('ubuntu', 'arch', ''):
            self.check_cli(spec, 'maintainer', choice)
            shutil.rmtree(self.bin)
            self.bin.mkdir()
        rows = self.fake_bin(spec)
        _, want, code = self.expected(rows, 'maintainer', 'arch')
        self.assertTrue({'FAIL  GCC 16: g++ 15.2, needs 16', 'FAIL  GCC 14: not found', 'OK    Python: python3 3.11', 'OK    git: git',
                         'WARN  PyPy (PyPy runs): not found', 'OK    Node.js (notebook page test): node', '  arch: AUR: gcc14 gcc14-libs',
                         'OK    role: maintainer'} <= set(want) and want[-1] == FAIL_LINE and code == 1, self.info(want=want, what='oracle sanity'))

    def test_cli_fake_required_pass(self):
        spec = {'g++': '', 'g++-14': '', 'python3': '3.13.1', 'rg': '', 'ccache': '', 'bwrap': '', 'lscpu': '', 'gh': '0',
                'systemd-run': '67108864', 'pypy3': '3.10.0', 'lualatex': '', 'kpsewhich': ''}
        self.check_cli(spec, 'contributor', 'arch', kpse_missing=('xcolor',))
        shutil.rmtree(self.bin)
        self.bin.mkdir()
        self.check_cli(spec, 'admin', 'ubuntu')

    def test_cli_main_labels(self):
        # main() in process with patched rows: platform and optional failures WARN, required and role failures FAIL
        import contextlib
        import io
        for plat, role, bad in (((False, 'arm'), (True, 'maintainer'), set()), ((True, 'x86'), (False, 'bad role'), set()),
                                ((False, 'arm'), (True, 'contributor'), {'PyPy', 'qemu-user'}), ((True, 'x86'), (True, 'contributor'), {'git', 'Node.js'}),
                                ((True, 'x86'), (True, 'contributor'), {'git', 'Node.js', 'GitHub CLI', 'PyPy'}),
                                ((True, 'x86'), (True, 'contributor'), {'GCC 14', 'GCC 16', 'GitHub CLI'}),
                                ((True, 'x86'), (True, 'contributor'), {'ccache', 'git', 'systemd user manager'})):
            # failure kinds: every failed row gets an install line except a probe problem of gh or systemd-run (configuration)
            rows = {t.name: (True, f'detail {t.name}') for t in S.TOOLS}
            for k, n in enumerate(sorted(bad)):
                rows[n] = (False, ['not found', 'x 1.0, needs 9', f'probe problem {n}'][k % 3])
            buf = io.StringIO()
            with patch.object(S, 'platform_check', return_value=plat), patch.object(S, 'role_check', return_value=role), \
                    patch.object(S, 'check', side_effect=lambda t: rows[t.name]), patch.object(sys, 'argv', ['setup_check.py', 'arch']), \
                    contextlib.redirect_stdout(buf):
                code = S.main()
            failed = not role[0] or any(TOOL[n].need is None for n in bad)
            missing = [t for t in S.TOOLS if t.name in bad
                       and not (rows[t.name][1].startswith('probe problem') and t.exes[0] in ('systemd-run', 'gh'))]
            want = [o_row('platform', None, *plat), *(o_row(t.name, t.need, *rows[t.name]) for t in S.TOOLS), o_row('role', None, *role),
                    *(['Install:', *o_install(missing, 'arch')] if missing else []), FAIL_LINE if failed else PASS_LINE]
            self.eq((code, buf.getvalue().splitlines()), (int(failed), want), platform=plat, role=role, bad=bad)
        self.eq(o_row('platform', None, False, 'arm')[:4], 'WARN', what='oracle sanity')

    def test_cli_bad_distro(self):
        r = self.run_cli('fedora')
        self.assertTrue(r.returncode == 2 and r.stdout == '' and 'fedora' in r.stderr, self.info(code=r.returncode, stdout=r.stdout, stderr=r.stderr))

    def test_cli_without_git(self):
        r = self.run_cli('arch', env=dict(ISOLATED, PATH=str(self.bin)))
        lines = r.stdout.splitlines()
        self.assertTrue(r.returncode == 1 and 'Traceback' not in r.stderr and 'FAIL  git: not found' in lines and lines[-1] == FAIL_LINE,
                        self.info(cmd=f'env PATH={self.bin} {sys.executable} {SETUP_PY} arch', code=r.returncode, stdout=r.stdout,
                                  stderr=r.stderr[-600:]))

    def test_cli_random(self):
        names = sorted(o_usepackages(NOTEBOOK_PY.read_text()))
        choices = {'g++': ['15.2.0', '16.0.1', '16', '17.1.0', 'garbage'], 'g++-14': ['14.3.0', '13.2', '15.1.0', '14'],
                   'python3': ['3.10.14', '3.11.0', '3.14.2', '2.7.18', '4.0', ''], 'pypy3': ['3.9.18', '3.10.14', '3.12.1', 'x'],
                   'gh': ['0', '1'], 'systemd-run': ['67108864', 'max', '134217728', '']}
        plain = ['rg', 'ccache', 'bwrap', 'lscpu', 'oj-bundle', 'kpsewhich', 'lualatex', 'node', 'nodejs', 'aarch64-linux-gnu-g++', 'qemu-aarch64']
        for r in range(self.rounds()):
            shutil.rmtree(self.bin)
            self.bin.mkdir()
            p = self.rng.random()
            spec = {e: self.rng.choice(v) for e, v in choices.items() if self.rng.random() < p}
            spec |= {e: '' for e in plain if self.rng.random() < p}
            missing = tuple(sorted(self.rng.sample(names, self.rng.randint(0, 2)))) if 'kpsewhich' in spec else ()
            self.check_cli(spec, self.rng.choice(['contributor', 'maintainer', 'admin']), self.rng.choice(['ubuntu', 'arch', '']),
                           kpse_missing=missing, round=r)

    def real_rows(self, r):
        lines = r.stdout.splitlines()
        self.assertTrue(lines and lines[-1] in (PASS_LINE, FAIL_LINE), self.info(stdout=r.stdout))
        self.eq(r.returncode, int(lines[-1] == FAIL_LINE), stdout=r.stdout, stderr=r.stderr)
        rows, rest = lines[:len(S.TOOLS) + 2], lines[len(S.TOOLS) + 2:-1]
        names = ['platform'] + [t.name for t in S.TOOLS] + ['role']
        needs = [None] + [t.need for t in S.TOOLS] + [None]
        failed = False
        for line, name, need in zip(rows, names, needs):
            label, head = line[:6].rstrip(), line[6:]
            prefix = name + (f' ({need})' if need else '') + ': '
            self.assertTrue(line[:6] in ('OK    ', 'WARN  ', 'FAIL  ') and head.startswith(prefix), self.info(line=line, name=name))
            self.eq(label, o_row(name, need, label == 'OK', '')[:6].rstrip(), line=line)
            failed |= label == 'FAIL'
        self.eq(len(rows), len(names), stdout=r.stdout)
        self.eq(failed, lines[-1] == FAIL_LINE, stdout=r.stdout)
        self.eq(rows[0].startswith('OK'), o_platform(), line=rows[0])
        missing = [t for t, line in zip(S.TOOLS, rows[1:-1]) if not line.startswith('OK')
                   and (line.endswith(': not found') or t.exes[0] not in ('gh', 'systemd-run'))]
        return missing, rows, rest

    def test_cli_real(self):
        r = self.run_cli(env=ISOLATED)
        missing, rows, rest = self.real_rows(r)
        where = o_distro(REAL_PATH('/etc/os-release').read_text()) if REAL_PATH('/etc/os-release').exists() else None
        self.eq(rest, ['Install:', *o_install(missing, where)] if missing else [], stdout=r.stdout, distro=where)
        path = ISOLATED.get('PATH', '')
        for t, line in zip(S.TOOLS, rows[1:-1]):
            present = [e for e in t.exes if o_which(e, path)]
            self.eq(line.endswith(': not found'), not present, line=line, present=present)
            if line.startswith('OK') and t.floor:
                code = 'import sys; print("%d.%d" % sys.version_info[:2])'
                cmd = [present[0], '-c', code] if len(t.floor) == 2 else [present[0], '-dumpfullversion']
                text = REAL_RUN(cmd, capture_output=True, text=True, env=ISOLATED).stdout.strip()
                have = o_version(text)
                self.assertTrue(have and have[:len(t.floor)] >= t.floor and (t.name != 'GCC 14' or have[0] == 14), self.info(line=line, version=text))
                self.eq(line, o_row(t.name, t.need, True, f'{present[0]} ' + '.'.join(map(str, have))), cmd=cmd)

    def test_cli_real_ubuntu(self):
        r = self.run_cli('ubuntu', env=ISOLATED)
        missing, _, rest = self.real_rows(r)
        self.eq(rest, ['Install:', *o_install(missing, 'ubuntu')] if missing else [], stdout=r.stdout)
        self.assertTrue(all(line.startswith('  ubuntu: ') for line in rest[1:]), self.info(stdout=r.stdout))


class Isa(Base):
    def setUp(self):
        super().setUp()
        RUNNER._triple.cache_clear()
        self.addCleanup(RUNNER._triple.cache_clear)
        self.log = self.tmp / 'log-triple'
        q = shlex.quote(str(self.log))
        self.x86 = sh(self.tmp / 'fake-x86', f'echo "$*" >> {q}\n[ "$1" = -dumpmachine ] && echo x86_64-pc-linux-gnu\n')
        self.arm = sh(self.tmp / 'fake-arm', '[ "$1" = -dumpmachine ] && echo aarch64-linux-gnu\n')
        self.fail = sh(self.tmp / 'fake-fail', 'exit 1\n')
        self.opt = sh(self.tmp / 'fake-opt', '[ "$1 $2" = "--opt -dumpmachine" ] && echo x86_64-linux-gnu\nexit 0\n')
        self.none = self.tmp / 'does-not-exist'
        self.bad = sh(self.tmp / 'fake-bad', 'echo x86_64-pc-linux-gnu\nexit 1\n')

    def test_isa_x86_triples(self):
        for compiler, want in ((self.x86, True), (self.arm, False), (self.fail, False), (self.none, False), (self.opt, False),
                              (self.bad, False)):
            self.eq(RUNNER.x86([str(compiler)]), want, compiler=compiler.name)
        self.eq(RUNNER._triple((str(self.arm),)), 'aarch64-linux-gnu')
        self.eq(RUNNER._triple((str(self.none),)), '')
        self.eq(RUNNER.x86([str(self.opt), '--opt']), True, compiler=[self.opt.name, '--opt'])

    def test_isa_native(self):
        # Windows reports AMD64 and macOS arm64 for the same architectures.
        for machine, host in (('x86_64', 'x86_64'), ('aarch64', 'aarch64'), ('AMD64', 'x86_64'), ('arm64', 'aarch64'), ('riscv64', 'riscv64')):
            for compiler, arch in ((self.x86, 'x86_64'), (self.arm, 'aarch64'), (self.fail, None), (self.none, None), (self.bad, None)):
                RUNNER._triple.cache_clear()
                with patch.object(RUNNER.platform, 'machine', return_value=machine):
                    self.eq(RUNNER.native([str(compiler)]), arch == host, compiler=compiler.name, machine=machine)
        for cxx, arch in ((self.x86, 'x86_64'), (self.arm, 'aarch64')):
            RUNNER._triple.cache_clear()
            with patch.dict(os.environ, {'CXX': str(cxx)}), patch.object(RUNNER.platform, 'machine', return_value='x86_64'):
                self.eq((RUNNER.native(), RUNNER.scaled(180)), (arch == 'x86_64', 180 if arch == 'x86_64' else 1800), cxx=cxx.name)

    def test_isa_scaled(self):
        for machine in ('x86_64', 'aarch64'):
            for compiler, arch in ((self.x86, 'x86_64'), (self.arm, 'aarch64'), (self.none, None)):
                RUNNER._triple.cache_clear()
                with patch.object(RUNNER.platform, 'machine', return_value=machine):
                    for seconds in (1, 180, 7):
                        self.eq(RUNNER.scaled(seconds, [str(compiler)]), seconds if arch == machine else 10 * seconds,
                                compiler=compiler.name, machine=machine, seconds=seconds)

    def test_isa_cache(self):
        for _ in range(3):
            self.eq(RUNNER.x86([str(self.x86)]), True)
        self.eq(self.log.read_text().splitlines(), ['-dumpmachine'], what='one -dumpmachine per compiler tuple')
        sh(self.x86, '[ "$1" = -dumpmachine ] && echo aarch64-linux-gnu\n')
        self.eq(RUNNER.x86([str(self.x86)]), True, what='cached triple')
        self.eq(RUNNER.x86([str(self.x86), '-m64']), False, what='another tuple is not cached')
        RUNNER._triple.cache_clear()
        self.eq(RUNNER.x86([str(self.x86)]), False, what='after cache_clear')

    def test_isa_default_cxx(self):
        with patch.dict(os.environ, {'CXX': f'{shlex.quote(str(self.opt))} --opt'}):
            self.eq(RUNNER.x86(), True, cxx=os.environ['CXX'])
            self.eq(RUNNER.x86_only(['-mavx2']), ['-mavx2'], cxx=os.environ['CXX'])
        with patch.dict(os.environ, {'CXX': str(self.arm)}):
            self.eq(RUNNER.x86(), False, cxx=str(self.arm))
        env = {k: v for k, v in os.environ.items() if k != 'CXX'} | {'PATH': str(self.bin)}
        for triple, want in (('x86_64-pc-linux-gnu', True), ('aarch64-linux-gnu', False)):
            RUNNER._triple.cache_clear()
            sh(self.bin / 'g++', f'[ "$1" = -dumpmachine ] && echo {triple}\n')
            with patch.dict(os.environ, env, clear=True):
                self.eq(RUNNER.x86(), want, cxx=None, triple=triple)

    def test_isa_x86_only(self):
        flags = ['-mavx2', '-mfma']
        got = RUNNER.x86_only(flags, [str(self.x86)])
        self.assertTrue(got == flags and got is not flags, self.info(got=got))
        got.append('-mbmi2')
        self.eq(flags, ['-mavx2', '-mfma'], what='copy')
        self.eq(RUNNER.x86_only(flags, [str(self.arm)]), [])
        self.eq(RUNNER.x86_only(flags, [str(self.none)]), [])

    def test_isa_cpu_runs_fake(self):
        cpu = 'processor\t: 0\nflags\t\t: fpu avx2 fma\n'
        with patch.object(RUNNER.platform, 'machine', return_value='x86_64'), patch.object(RUNNER, 'Path', fake_path('/proc/cpuinfo', cpu)):
            for features, compiler, want in (({'avx2', 'fma'}, self.x86, True), (set(), self.x86, True), ({'bmi2'}, self.x86, False),
                                             ({'avx2'}, self.arm, False), ({'avx2'}, self.none, False)):
                self.eq(RUNNER.cpu_runs(features, [str(compiler)]), want, features=features, compiler=compiler.name, cpuinfo=cpu)
        with patch.object(RUNNER.platform, 'machine', return_value='aarch64'), patch.object(RUNNER, 'Path', fake_path('/proc/cpuinfo', cpu)):
            self.eq(RUNNER.cpu_runs({'avx2'}, [str(self.x86)]), False, machine='aarch64')
        with patch.object(RUNNER.platform, 'machine', return_value='x86_64'), patch.object(RUNNER, 'Path', fake_path('/proc/cpuinfo', None)):
            self.eq(RUNNER.cpu_runs(set(), [str(self.x86)]), False, cpuinfo=None)

    def test_isa_cpu_runs_real(self):
        compiler = shlex.split(os.environ.get('CXX', 'g++'))
        try:
            triple = REAL_RUN([*compiler, '-dumpmachine'], capture_output=True, text=True, timeout=30).stdout.strip()
        except OSError:
            self.skipTest(f'SKIP: compiler {compiler} not found')
        cpu = REAL_PATH('/proc/cpuinfo')
        flags = o_cpu_flags(cpu.read_text()) if cpu.exists() else set()
        base = triple.startswith('x86_64') and platform.machine() in ('x86_64', 'AMD64') and cpu.exists()
        for features in ({'avx2', 'fma', 'bmi2'}, {'avx2'}, {'bmi2'}, {'avx512f'}, {'no-such-flag'}, set()):
            self.eq(RUNNER.cpu_runs(features), base and features <= flags, features=features, compiler=compiler, triple=triple)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')))
    parser.add_argument('--rounds', type=int, default=6, help='random rounds per random test in stress mode')
    args = parser.parse_args()
    MODE, SEED, ROUNDS = args.mode, args.seed, args.rounds
    suite = unittest.TestSuite()
    for case in (Tools, Version, Probe, Platform, Role, Distro, Install, Lint, Cli, Isa):
        suite.addTests(case(n) for n in unittest.defaultTestLoader.getTestCaseNames(case) if MODE != 'quick' or n not in FULL)
    print(f"setup checks: mode={MODE} seed={SEED} rounds={ROUNDS if MODE == 'stress' else 2 if MODE == 'full' else '-'} "
          f"tests={suite.countTestCases()}", flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
