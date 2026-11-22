#!/usr/bin/env python3
"""Notification hook (SUP32, matcher permission_prompt|idle_prompt): a desktop notification naming the session and why
it waits, through notify-send on Linux, terminal-notifier or osascript on macOS, and a PowerShell toast on Windows and
WSL. The session's name is its newest custom title in the transcript (--name, /rename), else the checkout's folder and
branch. In the terminal of VS Code or an editor built from it a click brings forward the window holding the session's
folder (osascript cannot). Silent when no notifier exists or it fails; it never blocks the session."""
import base64
import glob
import json
import os
import platform
import re
import shlex
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path, PurePosixPath, PureWindowsPath
from urllib.parse import quote, unquote, urlparse
from xml.sax.saxutils import escape

CHUNK = 1 << 20
WAIT = 600
DEADLINE = time.monotonic() + 9
APP = '--app-name=Claude Code'
TITLE = re.compile(r'"type":\s*"custom-title",\s*"customTitle":\s*("(?:[^"\\]|\\.)*")')
WHY = {'permission_prompt': 'waits for your permission', 'idle_prompt': 'waits for your input'}
AUMID = r'{1AC14E77-02E7-4E5D-B744-2EB1AE5198B7}\WindowsPowerShell\v1.0\powershell.exe'
WIN_PS = Path('/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe')
WIN_USERS = Path('/mnt/c/Users')
VSCODE = {'nameShort': 'Code', 'applicationName': 'code', 'urlProtocol': 'vscode'}  # without a readable product.json
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
            while end:
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


def editor():
    """(product, folder): the product.json values of the editor whose terminal runs the session (VS Code, Insiders,
    VSCodium, Cursor), found beside its built-in git extension's askpass script, and that folder; VS Code's values and
    None without one. A key the file lacks stays missing, so a click never opens another editor."""
    try:
        app = Path(os.environ['VSCODE_GIT_ASKPASS_MAIN']).parents[3]
        product = json.loads((app / 'product.json').read_text(encoding='utf-8'))
        return {k: v for k, v in product.items() if k in VSCODE and isinstance(v, str) and v}, app
    except (KeyError, IndexError, OSError, ValueError, AttributeError):
        return dict(VSCODE), None


def cli(product, app):
    """The editor's command-line launcher: in its own bin folder, else on PATH, or None."""
    if not (name := product.get('applicationName')):
        return None
    dirs = [app / 'bin', app.parents[1] / 'bin'] if app else []
    return next((str(p) for d in dirs if (p := d / name).is_file() and os.access(p, os.X_OK)), None) or shutil.which(name)


def folders(name):
    """The window folders the storage.json of the editor named name lists (Windows' copies from WSL), as this
    system's paths: file URIs, and on WSL the vscode-remote URIs of this distribution; none without a name."""
    if not name:
        return []
    sub = f'{name}/User/globalStorage/storage.json'
    root = {'linux': os.environ.get('XDG_CONFIG_HOME') or str(Path.home() / '.config'),
            'darwin': str(Path.home() / 'Library/Application Support'), 'win32': os.environ.get('APPDATA')}.get(SYSTEM)
    files = sorted(WIN_USERS.glob(f'*/AppData/Roaming/{glob.escape(sub)}')) if SYSTEM == 'wsl' else [Path(root, sub)] if root else []
    pure, distro, out = PureWindowsPath if SYSTEM == 'win32' else PurePosixPath, os.environ.get('WSL_DISTRO_NAME', ''), []
    for f in files:
        try:
            state = json.loads(f.read_text(encoding='utf-8'))['windowsState']
            uris = [w.get('folder') for w in [state.get('lastActiveWindow') or {}, *state.get('openedWindows', [])]]
        except (OSError, ValueError, KeyError, TypeError, AttributeError):
            continue
        for u in (urlparse(u) for u in uris if isinstance(u, str)):
            if u.scheme == 'file' and SYSTEM != 'wsl':
                out.append(pure(unquote(u.path).lstrip('/') if SYSTEM == 'win32' else unquote(u.path)))
            elif u.scheme == 'vscode-remote' and SYSTEM == 'wsl' and distro and unquote(u.netloc).lower() == f'wsl+{distro}'.lower():
                out.append(pure(unquote(u.path)))
    return out


def window(cwd, name):
    """The folder of the window holding cwd: the deepest window folder containing it, else the main checkout, so that
    a worktree's session finds its checkout's window, else cwd."""
    pure = PureWindowsPath if SYSTEM == 'win32' else PurePosixPath
    here = pure(Path(cwd).resolve())
    if held := [f for f in folders(name) if f == here or f in here.parents]:
        return max(held, key=lambda f: len(f.parts))
    common = git(cwd, 'rev-parse', '--path-format=absolute', '--git-common-dir')
    return pure(Path(common).parent) if Path(common).name == '.git' else pure(git(cwd, 'rev-parse', '--show-toplevel') or here)


def link(folder, product):
    """The URI a Windows toast opens for folder, or None without the editor's protocol or, on WSL, a distribution name."""
    if not (proto := product.get('urlProtocol')):
        return None
    if SYSTEM == 'wsl':
        distro = os.environ.get('WSL_DISTRO_NAME')
        return f'{proto}://vscode-remote/wsl+{quote(distro)}{quote(folder.as_posix())}' if distro else None
    return f'{proto}://file/' + quote(folder.as_posix().lstrip('/'), safe='/:')


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


def command(header, body, cwd, product, app):
    """The notifier's argv on macOS, Windows or WSL, a click focusing cwd's window in the editor product and app describe
    (product None: no click); None without a notifier. macOS opens the folder with the app bundle holding app, or
    with VS Code's bundle id when no product.json was found."""
    if SYSTEM == 'darwin':
        if tn := shutil.which('terminal-notifier'):
            mac = next((p for p in app.parents if p.suffix == '.app'), None) if app else None
            bundle = ['-a', str(mac)] if mac else [] if app else ['-b', 'com.microsoft.VSCode']
            folder = str(window(cwd, product.get('nameShort'))) if product and bundle else None
            click = ['-execute', shlex.join(['open', *bundle, folder])] if folder else []
            return [tn, '-title', header, '-message', body, '-group', header, *click]
        osa = shutil.which('osascript')
        return osa and [osa, '-e', 'on run argv', '-e', 'display notification (item 2 of argv) with title (item 1 of argv)',
                        '-e', 'end run', header, body]
    ps = shutil.which('powershell.exe') or shutil.which('powershell') or (str(WIN_PS) if SYSTEM == 'wsl' and WIN_PS.exists() else None)
    return ps and [ps, *toast(header, body, link(window(cwd, product.get('nameShort')), product) if product else None)]


def click(send, code, folder, header, body):
    """Detached helper on Linux: a notification with a default action; a click within WAIT seconds runs `code folder`.
    Variables naming the Claude Code session and inherited activation tokens are dropped, since a window `code` opens
    takes its environment."""
    p = subprocess.Popen([send, APP, '--action=default=Open', header, body], stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                         stderr=subprocess.DEVNULL, text=True)
    try:
        out = p.communicate(timeout=WAIT)[0]
    except subprocess.TimeoutExpired:  # SIGINT lets notify-send close its notification
        p.send_signal(signal.SIGINT)
        try:
            p.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            p.kill()
            p.wait()
        out = ''
    if 'default' in out.split():
        env = {k: v for k, v in os.environ.items() if not DROP.fullmatch(k)}
        subprocess.run([code, folder], env=env, stdin=subprocess.DEVNULL, capture_output=True, timeout=30)


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
    product, app = editor() if os.environ.get('TERM_PROGRAM') == 'vscode' else (None, None)
    if SYSTEM in ('darwin', 'win32', 'wsl'):
        argv = command(header, body, cwd, product, app)
    elif not (send := shutil.which('notify-send')):
        return
    else:
        argv = [send, APP, header, body]
        if product and (code := cli(product, app)):
            if '--action' in subprocess.run([send, '--help'], capture_output=True, text=True, timeout=left(2)).stdout:  # libnotify 0.7.10
                # CP_REAP_KEEP: /reap leaves this bounded helper alone, so that its notification never dies open
                subprocess.Popen([sys.executable, __file__, '--click', send, code, str(window(cwd, product.get('nameShort'))), header, body],
                                 start_new_session=True, env=os.environ | {'CP_REAP_KEEP': '1'}, stdin=subprocess.DEVNULL,
                                 stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                return
    if argv:
        subprocess.run(argv, capture_output=True, timeout=left(8))


if __name__ == '__main__':
    try:
        click(*sys.argv[2:7]) if sys.argv[1:2] == ['--click'] else main()
    except Exception:  # A notification must never disturb the session.
        pass
    sys.exit(0)
