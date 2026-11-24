#!/usr/bin/env python3
"""Notification hook (SUP32, matcher permission_prompt|idle_prompt): a desktop notification naming the session and why
it waits, through notify-send on Linux, terminal-notifier or osascript on macOS, and a PowerShell toast on Windows and
WSL. The session's name is its newest custom title in the transcript (--name, /rename), else the checkout's folder and
branch. In the terminal of VS Code or an editor built from it a click brings forward the window holding the session's
folder (osascript cannot) and, on Linux and Windows with .claude/vscode/terminal-focus installed, the session's terminal.
Silent when no notifier exists or it fails; it never blocks the session."""
import base64
import configparser
import glob
import json
import ntpath
import os
import platform
import posixpath
import re
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from pathlib import Path, PurePosixPath, PureWindowsPath
from urllib.parse import quote, unquote, urlparse
from xml.sax.saxutils import escape

CHUNK = 1 << 20
WAIT = 600
DEADLINE = time.monotonic() + 9
RESERVE = 3  # seconds of the deadline the title search leaves to the fallback name's git calls and the notifier
APP = '--app-name=Claude Code'
TITLE = re.compile(r'"type":\s*"custom-title",\s*"customTitle":\s*("(?:[^"\\]|\\.)*")')
WHY = {'permission_prompt': 'waits for your permission', 'idle_prompt': 'waits for your input'}
AUMID = r'{1AC14E77-02E7-4E5D-B744-2EB1AE5198B7}\WindowsPowerShell\v1.0\powershell.exe'


def wsl_drive(conf=Path('/etc/wsl.conf')):
    """Where WSL mounts the C: drive: <[automount] root of conf>/c, the root /mnt/ when conf or the key is missing."""
    c = configparser.ConfigParser(inline_comment_prefixes=('#', ';'), strict=False, interpolation=None)
    try:
        c.read_string(Path(conf).read_text(errors='replace'))
    except (OSError, configparser.Error):
        pass
    return Path(c.get('automount', 'root', fallback='/mnt/').strip().strip('"\'') or '/mnt/') / 'c'


WIN_PS = wsl_drive() / 'Windows/System32/WindowsPowerShell/v1.0/powershell.exe'
WIN_USERS = wsl_drive() / 'Users'
VSCODE = {'nameShort': 'Code', 'nameLong': 'Visual Studio Code', 'applicationName': 'code', 'urlProtocol': 'vscode',
          'dataFolderName': '.vscode'}  # without a readable product.json
PROC = Path('/proc')
HOPS = 64  # ancestors walked at most
EXTENSION = 'compprog.terminal-focus'  # <publisher>.<name> of .claude/vscode/terminal-focus
LIVE = []  # the helper's notify-send, closed when the helper is ended
JSONC = (re.compile(r'("(?:[^"\\]|\\.)*")|//[^\n]*|/\*.*?\*/', re.S), re.compile(r'("(?:[^"\\]|\\.)*")|,(?=\s*[\]}])'))
DROP = re.compile(r'CLAUDE.*|CP_USER|CP_REAP_KEEP|AI_AGENT|XDG_ACTIVATION_TOKEN|DESKTOP_STARTUP_ID|VSCODE_(?!IPC_HOOK_CLI$).*')


def detect(env, release, plat):
    """'wsl', 'darwin', 'win32' or another sys.platform value."""
    return 'wsl' if 'WSL_DISTRO_NAME' in env or 'microsoft' in release.lower() else plat


SYSTEM = detect(os.environ, platform.uname().release, sys.platform)


def left(cap):
    """Seconds a step may take: at most cap, within the hook's shared deadline."""
    return max(0.1, min(cap, DEADLINE - time.monotonic()))


def title(transcript):
    """The newest custom title in the transcript, searched from its end CHUNK bytes at a time, or ''."""
    try:
        with open(transcript, 'rb') as f:
            end, tail = f.seek(0, 2), []  # tail: the pieces after this chunk of the line it ends inside, nearest last
            while end and time.monotonic() < DEADLINE - RESERVE:  # the session's folder names it when time runs out
                start = max(end - CHUNK, 0)
                f.seek(start)
                chunk, end = f.read(end - start), start
                if start and b'\n' not in chunk:
                    tail.append(chunk)
                    continue
                cut = chunk.index(b'\n') + 1 if start else 0  # the line the chunk starts inside waits for the next chunk
                if found := TITLE.findall((chunk[cut:] + b''.join(reversed(tail))).decode('utf-8', 'replace')):
                    return json.loads(found[-1]).strip()
                tail = [chunk[:cut]]
        return ''
    except (OSError, TypeError, ValueError):
        return ''


def git(cwd, *args):
    try:
        return subprocess.run(['git', *args], cwd=cwd, capture_output=True, text=True, timeout=left(2)).stdout.strip()
    except (OSError, subprocess.TimeoutExpired):
        return ''


def checkout(cwd):
    """'<checkout folder> (<branch>)' for cwd, or its folder name outside git."""
    top = git(cwd, 'rev-parse', '--show-toplevel')
    branch = git(cwd, 'branch', '--show-current') if top else ''
    return Path(top or cwd).resolve().name + (f' ({branch})' if branch else '')


def product_dir(exe):
    """The folder of the product.json of an editor whose executable is exe: resources/app beside it (Linux), or its own
    folder (a remote server's node), else <build>/resources/app beside it (VS Code 1.140 on Windows keeps it in a folder
    named by its commit, live check B; the newest when an update left two, since the keys read never change); None for
    another program."""
    d = Path(exe).parent
    if a := next((a for a in (d / 'resources/app', d) if (a / 'product.json').is_file()), None):
        return a
    builds = [p.parent for p in d.glob('*/resources/app/product.json')]
    return max(builds, key=lambda a: (a / 'product.json').stat().st_mtime, default=None)


def option(line, name):
    """The value of --name in a command line read from /proc: NUL-separated arguments, or one string when the process
    rewrote its title, as Chromium's child processes do (the value then ends before the next ' --'); else None."""
    args = line.decode(errors='surrogateescape').rstrip('\0').split('\0')
    if len(args) == 1:
        m = re.search(rf'(?:^| )--{name}=(.+?)(?= --|$)', args[0])
        return m and m[1]
    for i, a in enumerate(args):
        if a.startswith(f'--{name}='):
            return a.partition('=')[2]
        if a == f'--{name}' and i + 1 < len(args):
            return args[i + 1]
    return None


def winprocs():
    """{pid: parent pid} of every process on Windows, from a Toolhelp snapshot; {} when it fails."""
    import ctypes
    from ctypes import wintypes

    class Entry(ctypes.Structure):
        _fields_ = [('dwSize', wintypes.DWORD), ('cntUsage', wintypes.DWORD), ('th32ProcessID', wintypes.DWORD),
                    ('th32DefaultHeapID', ctypes.c_size_t), ('th32ModuleID', wintypes.DWORD), ('cntThreads', wintypes.DWORD),
                    ('th32ParentProcessID', wintypes.DWORD), ('pcPriClassBase', ctypes.c_long), ('dwFlags', wintypes.DWORD),
                    ('szExeFile', ctypes.c_wchar * 260)]
    k = ctypes.WinDLL('kernel32', use_last_error=True)
    k.CreateToolhelp32Snapshot.restype, k.CreateToolhelp32Snapshot.argtypes = wintypes.HANDLE, [wintypes.DWORD, wintypes.DWORD]
    k.Process32FirstW.argtypes = k.Process32NextW.argtypes = [wintypes.HANDLE, ctypes.POINTER(Entry)]
    k.CloseHandle.argtypes = [wintypes.HANDLE]
    snap = k.CreateToolhelp32Snapshot(0x2, 0)  # TH32CS_SNAPPROCESS
    if not snap or snap == ctypes.c_void_p(-1).value:
        return {}
    out, e = {}, Entry()
    e.dwSize = ctypes.sizeof(Entry)
    try:
        ok = k.Process32FirstW(snap, ctypes.byref(e))
        while ok:
            out[e.th32ProcessID] = e.th32ParentProcessID
            ok = k.Process32NextW(snap, ctypes.byref(e))
    finally:
        k.CloseHandle(snap)
    return out


def winexe(pid):
    """The full executable path of process pid on Windows, or None when it cannot be opened."""
    import ctypes
    from ctypes import wintypes
    k = ctypes.WinDLL('kernel32', use_last_error=True)
    k.OpenProcess.restype, k.OpenProcess.argtypes = wintypes.HANDLE, [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    k.QueryFullProcessImageNameW.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.LPWSTR, ctypes.POINTER(wintypes.DWORD)]
    k.CloseHandle.argtypes = [wintypes.HANDLE]
    h = k.OpenProcess(0x1000, False, pid)  # PROCESS_QUERY_LIMITED_INFORMATION
    if not h:
        return None
    try:
        buf, size = ctypes.create_unicode_buffer(32768), wintypes.DWORD(32768)
        return buf.value if k.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(size)) else None
    finally:
        k.CloseHandle(h)


def winhost(pid=None, parents=None, exe=winexe):
    """host() on Windows, which has no /proc: walking up from pid through parents ({pid: parent pid}, default winprocs()),
    the first ancestor whose executable (exe(pid)) has a product.json, with 'shell' the ancestor below it and 'pid' the
    topmost one running the same executable. Other processes' environments and command lines are not read, so the walk
    neither stops at the terminal's edge nor finds --user-data-dir or --extensions-dir; {} without such an ancestor."""
    try:
        parents = winprocs() if parents is None else parents
        pid, below = pid or os.getpid(), None
        for _ in range(HOPS):
            if (path := exe(pid)) and (app := product_dir(path)):
                main = pid
                for _ in range(HOPS):
                    if (up := parents.get(main, 0)) <= 0 or exe(up) != path:
                        break
                    main = up
                return {'app': app, 'shell': below, 'pid': main, 'data': None, 'extensions': None}
            pid, below = parents.get(pid, 0), pid
            if pid <= 0:
                return {}
    except (OSError, ValueError, AttributeError):
        pass
    return {}


def host(pid=None):
    """The editor whose terminal runs this process (Linux and WSL; Windows through winhost()): walking up from pid (this process) through ancestors
    started with TERM_PROGRAM=vscode (the terminal's shell and what it runs), the first whose executable has a
    product.json. {'app': that folder, 'shell': the ancestor just below it (the terminal's shell), 'pid': the topmost
    ancestor running the same executable (the editor's main process, whose pid its windows carry), 'data' and
    'extensions': the first --user-data-dir and --extensions-dir from it upward}; {} without /proc, or when an ancestor
    outside the terminal comes first."""
    def exe(p):
        try:
            return os.readlink(PROC / str(p) / 'exe')
        except OSError:
            return None

    def parent(p):
        s = (PROC / str(p) / 'stat').read_bytes()
        return int(s[s.rindex(b')') + 2:].split()[1])
    if SYSTEM == 'win32':
        return winhost(pid)
    pid, below = pid or os.getpid(), None
    try:
        for _ in range(HOPS):
            if (path := exe(pid)) and (app := product_dir(path)):
                found, main = {'app': app, 'shell': below, 'data': None, 'extensions': None}, pid
                for _ in range(HOPS):
                    line = (PROC / str(main) / 'cmdline').read_bytes()
                    for key, flag in (('data', 'user-data-dir'), ('extensions', 'extensions-dir')):
                        found[key] = found[key] or option(line, flag)
                    if (up := parent(main)) <= 1 or exe(up) != path:
                        break
                    main = up
                return found | {'pid': main}
            if b'TERM_PROGRAM=vscode' not in (PROC / str(pid) / 'environ').read_bytes().split(b'\0'):
                return {}
            pid, below = parent(pid), pid
            if pid <= 1:
                return {}
    except (OSError, ValueError, IndexError):
        pass
    return {}


def editor():
    """(product, folder, host): the product.json values of the editor whose terminal runs the session (VS Code,
    Insiders, VSCodium, Cursor), that folder and host(). The editor is found from this process's ancestors (host()), else
    beside its built-in git extension's askpass script; VS Code's values and None without either. A key the file lacks
    stays missing, so a click never opens another editor."""
    found = host()
    try:
        app = found.get('app') or Path(os.environ['VSCODE_GIT_ASKPASS_MAIN']).parents[3]
        product = json.loads((app / 'product.json').read_text(encoding='utf-8'))
        return {k: v for k, v in product.items() if k in VSCODE and isinstance(v, str) and v}, app, found
    except (KeyError, IndexError, OSError, ValueError, AttributeError):
        return dict(VSCODE), None, found


def extensions(product, found):
    """The editor's extensions folder: its --extensions-dir, else $VSCODE_PORTABLE/extensions, else
    ~/<dataFolderName>/extensions; None without a dataFolderName."""
    if d := found.get('extensions'):
        return Path(d)
    if p := os.environ.get('VSCODE_PORTABLE'):
        return Path(p, 'extensions')
    return Path.home() / product['dataFolderName'] / 'extensions' if product.get('dataFolderName') else None


def focuser(product, found):
    """The URI that has the terminal-focus extension (.claude/vscode/terminal-focus) show the session's terminal: the
    terminal's shell, else this hook's parent, as pid; None unless the extension sits in the editor's extensions folder
    and the editor has a urlProtocol."""
    d = extensions(product, found)
    try:
        ok = d and product.get('urlProtocol') and any(
            (e.name == EXTENSION or e.name.startswith(EXTENSION + '-')) and (e / 'package.json').is_file() for e in d.iterdir())
    except OSError:
        return None
    return f"{product['urlProtocol']}://{EXTENSION}/focus?pid={found.get('shell') or os.getppid()}" if ok else None


def cli(product, app):
    """The editor's command-line launcher: in its own bin folder, else on PATH, or None."""
    if not (name := product.get('applicationName')):
        return None
    dirs = [app / 'bin', app.parents[1] / 'bin'] if app else []
    return next((str(p) for d in dirs if (p := d / name).is_file() and os.access(p, os.X_OK)), None) or shutil.which(name)


def userdata(name, data=None):
    """The user data folders of the editor named name: data (its --user-data-dir) when given, else
    $VSCODE_PORTABLE/user-data or $VSCODE_APPDATA/<name> when set (VS Code's order), else the platform's settings folder;
    on WSL every Windows user's roaming folder, data ignored. None without data or a name."""
    if SYSTEM == 'wsl':
        return sorted(WIN_USERS.glob(f'*/AppData/Roaming/{glob.escape(name)}')) if name else []
    if data:
        return [Path(data)]
    if not name:
        return []
    if portable := os.environ.get('VSCODE_PORTABLE'):
        return [Path(portable, 'user-data')]
    root = os.environ.get('VSCODE_APPDATA') or {'linux': os.environ.get('XDG_CONFIG_HOME') or str(Path.home() / '.config'),
                                                'darwin': str(Path.home() / 'Library/Application Support'),
                                                'win32': os.environ.get('APPDATA')}.get(SYSTEM)
    return [Path(root, name)] if root else []


def storage(name, data=None):
    """The storage.json files of the editor named name (userdata())."""
    return [d / 'User/globalStorage/storage.json' for d in userdata(name, data)]


def local(uri):
    """This system's path for a window's URI: a file URI, and on WSL a vscode-remote URI of this distribution; else None."""
    u, distro = urlparse(uri), os.environ.get('WSL_DISTRO_NAME', '')
    if u.scheme == 'file' and SYSTEM != 'wsl':
        if SYSTEM == 'win32':  # file://server/share/x is the UNC path \\server\share\x
            return PureWindowsPath(f'//{unquote(u.netloc)}{unquote(u.path)}' if u.netloc else unquote(u.path).lstrip('/'))
        return PurePosixPath(unquote(u.path))
    if u.scheme == 'vscode-remote' and SYSTEM == 'wsl' and distro and unquote(u.netloc).lower() == f'wsl+{distro}'.lower():
        return PurePosixPath(unquote(u.path))
    return None


def wsl_file(uri):
    """On WSL, this system's path of a Windows drive's file URI (file:///c%3A/x is <wsl_drive() parent>/c/x); else None.
    VS Code keeps untitled workspaces in the Windows user data folder, also for a WSL window (mainImpl.js 1.140)."""
    u = urlparse(uri)
    if SYSTEM != 'wsl' or u.scheme != 'file' or u.netloc or not (m := re.fullmatch(r'/([A-Za-z]):(/.*)?', unquote(u.path))):
        return None
    return PurePosixPath(WIN_USERS.parent.parent, m[1].lower(), (m[2] or '').lstrip('/'))


def workspace(path):
    """The folders a .code-workspace file lists (JSON with comments and trailing commas), `path` entries resolved against
    the file's folder."""
    text = Path(path).read_text(encoding='utf-8')
    for pattern in JSONC:
        text = pattern.sub(lambda m: m[1] or '', text)
    out, norm = [], ntpath.normpath if SYSTEM == 'win32' else posixpath.normpath
    for f in json.loads(text).get('folders') or []:
        if isinstance(f.get('path'), str):
            out.append(type(path)(norm(str(path.parent / f['path'].replace('\\', '/')))))
        elif isinstance(f.get('uri'), str) and (p := local(f['uri'])):
            out.append(p)
    return out


def folders(name, data=None):
    """The folders of windows(name, data)."""
    return [f for f, _ in windows(name, data)]


def windows(name, data=None):
    """(folder, file) for each folder of the editor named name (userdata()) as this system's paths, file being the saved
    .code-workspace of a workspace window holding it, else None: the folders of the windows its storage.json
    lists, a folder window's own and those of a workspace window's file, untitled workspaces
    (<user data>/Workspaces/<id>/workspace.json) included; none without a name or data. VS Code 1.140 rewrites the list
    within 100 ms of any of its windows losing focus and at shutdown (mainImpl.js), so it is current when a notification
    is clicked from elsewhere, except for a window whose folders changed while it was unfocused (code --add from another
    window), until it next loses focus; a closed untitled workspace, whose file stays, is never read."""
    out = []
    for f in storage(name, data):
        try:
            state = json.loads(f.read_text(encoding='utf-8'))['windowsState']
            windows = [state.get('lastActiveWindow') or {}, *state.get('openedWindows', [])]
        except (OSError, ValueError, KeyError, TypeError, AttributeError):
            continue
        for w in windows:
            try:
                ws = w.get('workspaceIdentifier') or {}
                uri = ws.get('configURIPath') if isinstance(ws, dict) else None
                if isinstance(w.get('folder'), str) and (p := local(w['folder'])):
                    out.append((p, None))
                elif isinstance(uri, str) and (p := local(uri) or wsl_file(uri)):
                    out += [(f, p if p.suffix == '.code-workspace' else None) for f in workspace(p)]
            except (OSError, ValueError, TypeError, AttributeError):
                continue
    return out


def window(cwd, name, data=None, file=False):
    """The folder of the window holding cwd: the deepest window folder (windows()) containing it, both compared with
    symbolic links resolved, as the window lists it, or with file its saved workspace file, which an opener needs since
    VS Code focuses an open window only on an exact folder or workspace match (mainImpl.js 1.140); else the main
    checkout, so that a worktree's session finds its checkout's window, else cwd."""
    pure = PureWindowsPath if SYSTEM == 'win32' else PurePosixPath
    real = (lambda f: pure(os.path.realpath(f))) if (os.name == 'nt') == (SYSTEM == 'win32') else pure  # paths of this OS only
    here = pure(Path(cwd).resolve())
    if held := [(r, f, w) for f, w in windows(name, data) if (r := real(f)) == here or r in here.parents]:
        _, f, w = max(held, key=lambda h: len(h[0].parts))
        return w if file and w else f
    common = git(cwd, 'rev-parse', '--path-format=absolute', '--git-common-dir')
    return pure(Path(common).parent) if Path(common).name == '.git' else pure(git(cwd, 'rev-parse', '--show-toplevel') or here)


def link(folder, product):
    """The URI a Windows toast opens for folder, or None without the editor's protocol or, on WSL, a distribution name."""
    if not (proto := product.get('urlProtocol')):
        return None
    if SYSTEM == 'wsl':
        distro = os.environ.get('WSL_DISTRO_NAME')
        return f'{proto}://vscode-remote/wsl+{quote(distro)}{quote(folder.as_posix())}' if distro else None
    return f'{proto}://file' + uri_path(folder)


def uri_path(folder):
    """folder's path as a URI writes it: a UNC path keeps its // (vscode://file//server/share/x), any other starts with
    one /; '#', '?', '%' and spaces quoted."""
    path = folder.as_posix()
    return quote(path if path.startswith('//') else '/' + path.lstrip('/'), safe='/:')


def toast(header, body, uri):
    """powershell.exe arguments showing a Windows toast that opens uri when clicked (uri None: no click action). The XML
    holds only ASCII without quote marks, since PowerShell also ends a single-quoted string at U+2018 to U+201B."""
    def esc(s):
        return ''.join(c if c.isascii() and c != "'" else f'&#{ord(c)};' for c in escape(s, {'"': '&quot;'}))
    launch = f' activationType="protocol" launch="{esc(uri)}"' if uri else ''
    xml = (f'<toast{launch}><visual><binding template="ToastGeneric"><text>{esc(header)}</text><text>{esc(body)}</text>'
           '</binding></visual></toast>')
    ps = ('[Windows.UI.Notifications.ToastNotificationManager, Windows.UI.Notifications, ContentType = WindowsRuntime] > $null\n'
          '$x = [Windows.Data.Xml.Dom.XmlDocument, Windows.Data.Xml.Dom.XmlDocument, ContentType = WindowsRuntime]::new()\n'
          f"$x.LoadXml('{xml}')\n"
          f"[Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier('{AUMID}')"
          '.Show([Windows.UI.Notifications.ToastNotification]::new($x))')
    return ['-NoProfile', '-NonInteractive', '-EncodedCommand', base64.b64encode(ps.encode('utf-16-le')).decode()]


def command(header, body, cwd, product, app, found=None):
    """The notifier's argv on macOS, Windows or WSL, a click focusing cwd's window in the editor product and app describe
    (product None: no click); None without a notifier. macOS opens the folder with the app bundle holding app, or
    with VS Code's bundle id when no product.json was found. On Windows with the terminal-focus extension installed the
    toast opens its URI (focuser()) with the folder, which shows the session's terminal in the window holding it."""
    if SYSTEM == 'darwin':
        if tn := shutil.which('terminal-notifier'):
            mac = next((p for p in app.parents if p.suffix == '.app'), None) if app else None
            bundle = ['-a', str(mac)] if mac else [] if app else ['-b', 'com.microsoft.VSCode']
            folder = str(window(cwd, product.get('nameShort'), file=True)) if product and bundle else None
            click = ['-execute', shlex.join(['open', *bundle, folder])] if folder else []
            return [tn, '-title', header, '-message', body, '-group', header, *click]
        osa = shutil.which('osascript')
        return osa and [osa, '-e', 'on run argv', '-e', 'display notification (item 2 of argv) with title (item 1 of argv)',
                        '-e', 'end run', header, body]
    ps = shutil.which('powershell.exe') or shutil.which('powershell') or (str(WIN_PS) if SYSTEM == 'wsl' and WIN_PS.exists() else None)
    uri = None
    if product:
        folder = window(cwd, product.get('nameShort'), file=True)
        ext = SYSTEM == 'win32' and folder.is_absolute() and focuser(product, found or {})
        uri = f"{ext}&folder={quote('file://' + uri_path(folder).removeprefix('//'), safe='')}" if ext else link(folder, product)
    return ps and [ps, *toast(header, body, uri)]


KWIN = """const path = " - " + %s + " - ", name = " - " + %s + " - ", app = %s, pid = %d, mark = %s;
const list = workspace.windowList ? workspace.windowList() : workspace.clientList();
let best = null, top = 0, all = 0, byPid = 0, byPath = 0, byName = 0;
for (const w of list) {
    if (!w.normalWindow) continue;
    const t = " - " + w.caption + " - ", titled = app !== "" && w.caption.endsWith(app);
    const s = (pid > 0 && w.pid === pid ? 4 : 0) + (titled && t.includes(path) ? 2 : 0) + (titled && t.includes(name) ? 1 : 0);
    all += s > 0; byPid += (s & 4) > 0; byPath += (s & 2) > 0; byName += (s & 1) > 0;
    if (s > top) { best = w; top = s; }
}
if (mark) {
    print(mark + " " + all + " " + byPid + " " + byPath + " " + byName + " " + (best ? best.caption : ""));
} else if (best) {
    best.minimized = false;
    if ("activeWindow" in workspace) { workspace.activeWindow = best; } else { workspace.activeClient = best; }
}
"""


def tildify(path):
    """path as a window title shows ${rootPath}: under the home folder as ~/<rest>, else unchanged (VS Code 1.140's
    tildify on Linux)."""
    home = str(Path.home()).rstrip('/') + '/'
    return '~/' + path[len(home):] if path.startswith(home) else path


def script(folder, app, pid, mark=''):
    """The KWin script choosing the editor window for folder: a normal window scores 4 when its pid is the editor's main
    process, 2 when its title holds ' - <folder as tildify() shows it> - ' and 1 when it holds ' - <folder name> - ', each
    title match only when the title ends with app (the editor's nameLong; the project's window.title shows the path, VS
    Code's default the name); the first of the highest score wins. Without mark it unminimizes and activates that window;
    with mark it only prints mark, the counts of matching windows (any, by pid, by path, by name) and the winner's title."""
    return KWIN % (json.dumps(tildify(str(folder))), json.dumps(PurePosixPath(folder).name), json.dumps(app or ''), int(pid or 0),
                   json.dumps(mark))


def kwin(text, plugin):
    """Loads, runs and unloads a one-shot KWin script over D-Bus (gdbus, else qdbus6 or qdbus) as plugin; False without
    a D-Bus tool or when loading fails."""
    gdbus = shutil.which('gdbus')
    qdbus = None if gdbus else shutil.which('qdbus6') or shutil.which('qdbus')
    if not (gdbus or qdbus):
        return False

    def call(path, method, *args):
        argv = ([gdbus, 'call', '--session', '--dest', 'org.kde.KWin', '--object-path', path, '--method', method] if gdbus
                else [qdbus, 'org.kde.KWin', path, method])
        return subprocess.run([*argv, *args], capture_output=True, text=True, timeout=10).stdout
    fd, path = tempfile.mkstemp(prefix='claude-notify-', suffix='.js')
    try:
        with os.fdopen(fd, 'w') as f:
            f.write(text)
        if m := re.search(r'-?\d+', call('/Scripting', 'org.kde.kwin.Scripting.loadScript', path, plugin)):
            call(f'/Scripting/Script{m[0]}', 'org.kde.kwin.Script.run')
        call('/Scripting', 'org.kde.kwin.Scripting.unloadScript', plugin)
        return bool(m)
    finally:
        os.unlink(path)


def activate(folder, app, pid=None):
    """On KDE, have KWin activate the editor's window for folder (script()): a native Wayland window cannot raise itself
    without an activation token, which `code` cannot pass on to the running editor. Nothing without app or pid."""
    if (app or pid) and 'KDE' in os.environ.get('XDG_CURRENT_DESKTOP', '').split(':'):
        kwin(script(folder, app, pid), f'claude-notify-{os.getpid()}')


def stop(p):
    """SIGINT, so that notify-send closes its notification, then SIGKILL after 5 s."""
    p.send_signal(signal.SIGINT)
    try:
        p.wait(timeout=5)
    except subprocess.TimeoutExpired:
        p.kill()
        p.wait()


def end(*_):
    """The helper's SIGTERM and SIGHUP handler (/reap, the SessionEnd hook): close the notification, then exit."""
    for p in LIVE:
        stop(p)
    os._exit(0)


def click(send, code, folder, header, body, app='', pid='', uri='', data='', ext=''):
    """Detached helper on Linux: a notification with a default action; a click within WAIT seconds runs
    activate(folder, app, pid), then `code folder`, then `code --open-url uri` (the terminal-focus extension) when given,
    each with the editor's --user-data-dir data and --extensions-dir ext when given, so they reach the running editor.
    Variables naming the Claude Code session and inherited activation tokens are dropped, since a window `code` opens
    takes its environment."""
    p = subprocess.Popen([send, APP, '--action=default=Open', header, body], stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                         stderr=subprocess.DEVNULL, text=True)
    LIVE.append(p)
    try:
        out = p.communicate(timeout=WAIT)[0]
    except subprocess.TimeoutExpired:
        stop(p)
        out = ''
    if 'default' in out.split():
        env = {k: v for k, v in os.environ.items() if not DROP.fullmatch(k)}
        activate(folder, app, pid)  # first: `code` takes about a second, and only opens a window when none holds folder
        own = [*(['--user-data-dir', data] if data else []), *(['--extensions-dir', ext] if ext else [])]
        for argv in ([code, *own, folder], [code, *own, '--open-url', uri]) if uri else ([code, *own, folder],):
            subprocess.run(argv, env=env, stdin=subprocess.DEVNULL, capture_output=True, timeout=30)


def main():
    try:
        data = json.load(sys.stdin)
    except ValueError:
        data = {}
    data = data if isinstance(data, dict) else {}
    cwd = data.get('cwd') if isinstance(data.get('cwd'), str) and Path(data['cwd']).is_dir() else '.'
    name = title(data.get('transcript_path')) or checkout(cwd)
    kind = data.get('notification_type')
    message = data.get('message') if isinstance(data.get('message'), str) else ''
    body = (WHY.get(kind) if isinstance(kind, str) else None) or 'needs attention'
    body = (body + (f': {message}' if message else ''))[:300]
    header = f'Claude Code · {name}'[:120]
    product, app, found = editor() if os.environ.get('TERM_PROGRAM') == 'vscode' else (None, None, {})
    if SYSTEM in ('darwin', 'win32', 'wsl'):
        argv = command(header, body, cwd, product, app, found)
    elif not (send := shutil.which('notify-send')):
        return
    else:
        argv = [send, APP, header, body]
        if product and (code := cli(product, app)):
            if '--action' in subprocess.run([send, '--help'], capture_output=True, text=True, timeout=left(2)).stdout:  # libnotify 0.7.10
                # no CP_REAP_KEEP: /reap --own and the SessionEnd hook end this helper as the session's leftover, and its
                # SIGTERM handler (end()) closes the notification first
                folder = window(cwd, product.get('nameShort'), found.get('data'))
                subprocess.Popen([sys.executable, __file__, '--click', send, code, str(folder), header, body, product.get('nameLong', ''),
                                  str(found.get('pid') or ''), focuser(product, found) or '', found.get('data') or '',
                                  found.get('extensions') or ''],
                                 start_new_session=True, env={k: v for k, v in os.environ.items() if k != 'CP_REAP_KEEP'}, stdin=subprocess.DEVNULL,
                                 stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                return
    if argv:
        subprocess.run(argv, capture_output=True, timeout=left(8))


if __name__ == '__main__':
    try:
        if sys.argv[1:2] == ['--click']:
            for sig in (signal.SIGTERM, signal.SIGHUP):
                signal.signal(sig, end)
            click(*sys.argv[2:12])
        else:
            main()
    except Exception:  # A notification must never disturb the session.
        pass
    sys.exit(0)
