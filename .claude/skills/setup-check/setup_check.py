#!/usr/bin/env python3
"""Checks this machine for the tools the repository runs: setup_check.py [ubuntu | arch] [--lint].

TOOLS declares each tool once: what needs it (required, or the optional feature), its executables, its floor, and its
Ubuntu 26.04 and Arch packages. Probes compile, link and run where a version proves nothing (sanitizers, static libc,
the systemd memory cap, the arm64 emulator). A required failure exits 1; an optional one warns. Both print the install
command for this distribution (the argument, else /etc/os-release). --lint lists every executable a tracked Python script
runs by a literal name (the first item of a subprocess command, or a shutil.which argument) that TOOLS does not declare,
and exits 1 when there is one; 03-consistency.py runs it.
"""
import argparse
import ast
import collections
import functools
import importlib.util
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
Tool = collections.namedtuple('Tool', 'name need exes floor ubuntu arch')
TOOLS = [  # need: None when required, else the optional feature; exes: alternatives, the first found is used
    Tool('GCC 16', None, ('g++',), (16,), 'g++-16 && mkdir -p ~/.local/bin && ln -sf /usr/bin/g++-16 ~/.local/bin/g++', 'gcc'),
    Tool('GCC 14', None, ('g++-14',), (14,), 'g++-14', 'AUR: gcc14 gcc14-libs'),
    Tool('Python', None, ('python3',), (3, 11), 'python3', 'python'),  # the scripts' floor: tomllib (07-judge.py)
    Tool('git', None, ('git',), None, 'git', 'git'),
    Tool('ripgrep', None, ('rg',), None, 'ripgrep', 'ripgrep'),
    Tool('ccache', None, ('ccache',), None, 'ccache', 'ccache'),
    Tool('bubblewrap', None, ('bwrap',), None, 'bubblewrap', 'bubblewrap'),
    Tool('systemd user manager', None, ('systemd-run',), None, 'systemd', 'systemd'),
    Tool('lscpu', None, ('lscpu',), None, 'util-linux', 'util-linux'),
    Tool('GitHub CLI', None, ('gh',), None, 'gh', 'github-cli'),
    Tool('LuaLaTeX', 'notebook PDF', ('lualatex',), None, 'texlive-luatex texlive-latex-recommended',
         'texlive-luatex texlive-latexrecommended'),
    Tool('kpathsea', 'notebook PDF', ('kpsewhich',), None, 'texlive-binaries', 'texlive-bin'),
    Tool('fontconfig', 'notebook PDF', ('fc-list',), None, 'fontconfig fonts-inter fonts-jetbrains-mono',
         'fontconfig inter-font ttf-jetbrains-mono'),
    Tool('Node.js', 'notebook page test', ('node', 'nodejs'), None, 'nodejs', 'nodejs'),
    Tool('site engine', 'site', ('zensical', 'mkdocs'), None, 'pipx && pipx install zensical',
         'python-pipx && pipx install zensical'),
    Tool('oj-bundle', 'online expansion', ('oj-bundle',), None, 'pipx && pipx install online-judge-verify-helper',
         'python-pipx && pipx install online-judge-verify-helper'),
    Tool('PyPy', 'PyPy runs', ('pypy3',), (3, 10), 'pypy3', 'pypy3'),
    Tool('arm64 cross g++', 'arm64 check', ('aarch64-linux-gnu-g++',), None, 'g++-aarch64-linux-gnu',
         'aarch64-linux-gnu-gcc'),
    Tool('qemu-user', 'arm64 check', ('qemu-aarch64',), None, 'qemu-user qemu-user-binfmt', 'qemu-user qemu-user-binfmt'),
]
LD_PREFIX = '/usr/aarch64-linux-gnu'  # arm64 sysroot of the cross packages on both distributions, for QEMU_LD_PREFIX
PY_VERSION = 'import sys; print(*sys.version_info[:3], sep=".")'
CONFIGURED = ('systemd-run', 'gh')  # their probes check configuration (delegation, login), which no package fixes
SUBPROCESS = {'run', 'Popen', 'call', 'check_call', 'check_output'}


@functools.cache
def module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def out(cmd, **kw):
    """(return code, stdout + stderr) of cmd, or (None, reason) when it cannot run or exceeds 120 s."""
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=120, **kw)
    except (OSError, subprocess.TimeoutExpired) as e:
        return None, str(e)
    return r.returncode, (r.stdout + r.stderr).strip()


def version(tool, exe):
    """Version tuple of exe, or None when it prints none."""
    cmd = [exe, '-c', PY_VERSION] if tool.floor and len(tool.floor) == 2 else [exe, '-dumpfullversion']
    match = re.match(r'(\d+)(?:\.(\d+))?', out(cmd)[1] or '')
    return tuple(int(g) for g in match.groups() if g) if match else None


def capped(cmd):
    """cmd under a 256 MB memory cap when systemd user scopes work (probes are compiled programs), else cmd."""
    cap = module('setup_memory_cap', '96-Local Testing/_00_memory_cap.py')
    return cap.capped(cmd, 256) if cap.available() else cmd


def compiles(cxx, flags, run=True, env=None):
    """None when a small C++ program builds with flags (and runs, if run), else why not."""
    with tempfile.TemporaryDirectory(prefix='cp-setup-') as d:
        src, exe = Path(d, 'p.cpp'), str(Path(d, 'p'))
        src.write_text('#include <bits/stdc++.h>\nint main(int c, char**) { std::vector<int> v(c, 7); '
                       'return std::accumulate(v.begin(), v.end(), 0) == 7 ? 0 : 1; }\n')
        code, text = out([cxx, '-std=gnu++20', *flags, str(src), '-o', exe])
        if code != 0:
            return 'build: ' + (text.splitlines() or ['failed'])[-1]
        if run:
            code, text = out(capped([exe]), env=env)
            return None if code == 0 else f'run exit {code}: ' + (text.splitlines() or [''])[-1]
    return None


def probe(tool, exe):
    """Problems found beyond presence and floor (functional probes), as short strings."""
    if tool.exes[0] in ('g++', 'g++-14'):
        return [f'{label} {why}' for label, flags in (('ASan/UBSan', ['-fsanitize=address,undefined']), ('static', ['-static']))
                if (why := compiles(exe, flags))]
    if exe == 'systemd-run':  # the scope must apply MemoryMax: cgroup v2 with the memory controller delegated
        code, text = out(['systemd-run', '--user', '--scope', '--quiet', '--expand-environment=no', '-p', 'MemoryMax=64M', 'sh', '-c',
                          'cat "/sys/fs/cgroup$(sed -n "s/^0:://p" /proc/self/cgroup)/memory.max"'])
        return [] if text == str(64 << 20) else [f'no memory-capped user scope ({text or code}): enable a systemd user '
                                                 'session with cgroup v2 and the memory controller delegated']
    if exe == 'gh':
        return [] if out(['gh', 'auth', 'status'])[0] == 0 else ['not logged in: gh auth login']
    if exe == 'lualatex':
        text = (ROOT / '98-Team Notebook/01-notebook.py').read_text()
        names = {n.strip() for group in re.findall(r'usepackage(?:\[[^]]*\])?\{([^}]*)\}', text) for n in group.split(',')}
        return [f'TeX package {n} missing' for n in sorted(names) if shutil.which('kpsewhich') and out(['kpsewhich', n + '.sty'])[0]]
    if exe == 'fc-list':
        book = module('setup_notebook', '98-Team Notebook/01-notebook.py')
        cfg = book.load(book.sources())
        return [f'font {cfg[k]!r} missing' for k in ('body_font', 'code_font') if book.font_status(cfg[k]) is False]
    if tool.name == 'site engine':
        found = module('setup_site', '98-Team Notebook/06-site.py').engine()
        return [found] if isinstance(found, str) else []
    if exe == 'qemu-aarch64':
        cross = shutil.which('aarch64-linux-gnu-g++')
        if not cross:
            return ['needs aarch64-linux-gnu-g++ to build its arm64 probe']
        why = compiles(cross, [], env=dict(os.environ, QEMU_LD_PREFIX=LD_PREFIX))
        return [f'arm64 binary does not run directly ({why}): register binfmt (qemu-user-binfmt)'] if why else []
    return []


def check(tool):
    """(ok, detail) of one tool."""
    exe = next((e for e in tool.exes if shutil.which(e)), None)
    if exe is None:
        return False, 'not found'
    have = version(tool, exe) if tool.floor else None
    if tool.floor and (have is None or have[:len(tool.floor)] < tool.floor or (tool.name == 'GCC 14' and have[0] != 14)):
        return False, f"{exe} {'.'.join(map(str, have or ())) or 'unknown version'}, needs {'.'.join(map(str, tool.floor))}"
    problems = probe(tool, exe)
    return not problems, '; '.join(problems) or (exe + (' ' + '.'.join(map(str, have)) if have else ''))


def platform_check():
    """(ok, detail): Core full packages measure and test their AVX2/FMA/BMI2 kernels on x86-64."""
    if platform.machine() not in ('x86_64', 'AMD64'):
        return False, f'{platform.machine()}: ISA paths skip; Core full packages need x86-64 with AVX2, FMA and BMI2'
    cpu = Path('/proc/cpuinfo')
    lacks = sorted({'avx2', 'fma', 'bmi2'} - set(cpu.read_text().split() if cpu.exists() else ()))
    return not lacks, 'x86-64 lacking ' + ', '.join(lacks) if lacks else 'x86-64 with AVX2, FMA and BMI2'


def role_check():
    if not shutil.which('git'):
        return False, 'needs git'
    plan = module('setup_plan', '00-Guidelines/13-Plan/plan.py')
    try:
        return True, plan.role(ROOT)
    except SystemExit as e:
        return False, str(e)


def distro(choice):
    if choice:
        return choice
    try:
        ids = re.findall(r"""^ID(?:_LIKE)?=["']?([^"'\n]*)""", Path('/etc/os-release').read_text(), re.M)
    except OSError:
        return None
    words = ' '.join(ids).split()
    return 'arch' if 'arch' in words else 'ubuntu' if {'ubuntu', 'debian'} & set(words) else None


def install(tools, where):
    """Install commands for tools on distribution where (both when unknown): one for the plain packages, then one per
    tool that needs more steps."""
    lines = []
    for d, cmd in (('ubuntu', 'sudo apt install '), ('arch', 'sudo pacman -S ')):
        if where in (None, d):
            pkgs = [t.ubuntu if d == 'ubuntu' else t.arch for t in tools]
            plain = [p for p in pkgs if '&&' not in p and not p.startswith('AUR')]
            lines += [f'  {d}: {cmd}' + ' '.join(plain)] if plain else []
            lines += [f'  {d}: ' + (p if p.startswith('AUR') else cmd + p) for p in pkgs if p not in plain]
    return lines


def commands(path):
    """Executables a Python file runs by a literal name: subprocess command heads (first argument or args=; an
    os.environ.get or os.getenv default counts) and shutil.which arguments, through any import alias."""
    tree = ast.parse(path.read_bytes())
    names, alias = set(), {}  # alias: local name -> 'subprocess' or 'shutil' (modules), or 'subprocess.run' (functions)
    for n in ast.walk(tree):
        if isinstance(n, ast.Import):
            alias.update({a.asname or a.name: a.name for a in n.names if a.name in ('subprocess', 'shutil')})
        elif isinstance(n, ast.ImportFrom) and n.module in ('subprocess', 'shutil'):
            alias.update({a.asname or a.name: f'{n.module}.{a.name}' for a in n.names})
    for n in ast.walk(tree):
        if not isinstance(n, ast.Call):
            continue
        f = n.func
        name = (f'{alias.get(f.value.id)}.{f.attr}' if isinstance(f, ast.Attribute) and isinstance(f.value, ast.Name)
                else alias.get(f.id) if isinstance(f, ast.Name) else None)
        a = n.args[0] if n.args else next((k.value for k in n.keywords if k.arg in ('args', 'cmd')), None)
        if name == 'shutil.which' and a is not None:
            names.add(a)
        elif name and name.startswith('subprocess.') and name[11:] in SUBPROCESS and isinstance(a, (ast.List, ast.Tuple)) and a.elts:
            head = a.elts[0]
            if isinstance(head, ast.Call) and ast.unparse(head.func) in ('os.environ.get', 'os.getenv') and len(head.args) == 2:
                head = head.args[1]
            names.add(head)
    return {n.value for n in names if isinstance(n, ast.Constant) and isinstance(n.value, str)}


def undeclared(root=ROOT):
    """(file, executable) pairs for tracked scripts outside OLD and 95-Resources running a tool TOOLS lacks."""
    files = subprocess.run(['git', 'ls-files', '-z', '--', '*.py'], cwd=root, capture_output=True, text=True,
                           check=True).stdout.split('\0')
    declared = {e for t in TOOLS for e in t.exes}
    return [(f, name) for f in sorted(files) if f and not f.startswith(('OLD/', '95-Resources/')) and (root / f).is_file()
            for name in sorted(commands(root / f)) if Path(name).name not in declared]


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('distro', nargs='?', default='', help='ubuntu or arch: install hints for it (default: detected)')
    parser.add_argument('--lint', action='store_true', help='list tools scripts run that TOOLS does not declare')
    args = parser.parse_args()
    if args.distro not in ('', 'ubuntu', 'arch'):
        parser.error(f'distribution must be ubuntu or arch, not {args.distro!r}')
    if args.lint:
        found = undeclared()
        for f, name in found:
            print(f'UNDECLARED {name} in {f}')
        return int(bool(found))
    missing, failed = [], False
    rows = [(None, 'platform', None, platform_check()), *((t, t.name, t.need, check(t)) for t in TOOLS),
            (None, 'role', None, role_check())]
    for tool, name, need, (ok, detail) in rows:
        label = 'OK' if ok else 'WARN' if need or name == 'platform' else 'FAIL'
        failed |= label == 'FAIL'
        print(f'{label:4}  {name}{f" ({need})" if need else ""}: {detail}')
        if tool and not ok and (detail == 'not found' or tool.exes[0] not in CONFIGURED):
            missing.append(tool)
    if missing:
        print('Install:', *install(missing, distro(args.distro)), sep='\n')
    print('FAIL: a required tool is missing or below its floor' if failed else 'PASS: every required tool works')
    return int(failed)


if __name__ == '__main__':
    raise SystemExit(main())
