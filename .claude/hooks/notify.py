#!/usr/bin/env python3
"""Notification hook (SUP32, matcher permission_prompt|idle_prompt): a desktop notification naming the session and why
it waits, through notify-send on Linux, terminal-notifier or osascript on macOS, and a PowerShell toast on Windows and
WSL. The session's name is its newest custom title in the transcript (--name, /rename), else the checkout's folder and
branch. In VS Code's terminal a click brings forward the window holding the session's folder (osascript cannot). Silent
when no notifier exists or it fails; it never blocks the session."""
import base64
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
from pathlib import Path
from urllib.parse import quote, unquote, urlparse
from xml.sax.saxutils import escape

TAIL = 1 << 20
WAIT = 600
DEADLINE = time.monotonic() + 9
APP = '--app-name=Claude Code'
TITLE = re.compile(r'"type":\s*"custom-title",\s*"customTitle":\s*("(?:[^"\\]|\\.)*")')
WHY = {'permission_prompt': 'waits for your permission', 'idle_prompt': 'waits for your input'}
AUMID = r'{1AC14E77-02E7-4E5D-B744-2EB1AE5198B7}\WindowsPowerShell\v1.0\powershell.exe'
WIN_PS = Path('/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe')
DROP = re.compile(r'CLAUDE.*|CP_USER|AI_AGENT|XDG_ACTIVATION_TOKEN|DESKTOP_STARTUP_ID|VSCODE_(?!IPC_HOOK_CLI$).*')


def detect(env, release, plat):
    """'wsl', 'darwin', 'win32' or another sys.platform value."""
    return 'wsl' if 'WSL_DISTRO_NAME' in env or 'microsoft' in release.lower() else plat


SYSTEM = detect(os.environ, platform.uname().release, sys.platform)
STORAGE = {'linux': Path(os.environ.get('XDG_CONFIG_HOME') or Path.home() / '.config'),
           'darwin': Path.home() / 'Library/Application Support'}.get(SYSTEM)


def left(cap):
    """Seconds a step may take: at most cap, within the hook's shared deadline."""
    return max(0.1, min(cap, DEADLINE - time.monotonic()))


def title(transcript):
    """The newest custom title within the transcript's last MiB, or ''."""
    try:
        with open(transcript, 'rb') as f:
            f.seek(max(f.seek(0, 2) - TAIL, 0))
            found = TITLE.findall(f.read().decode('utf-8', 'replace'))
        return json.loads(found[-1]).strip() if found else ''
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


def window(cwd):
    """The folder of the VS Code window holding cwd: the deepest open window folder containing it (Linux and macOS
    keep the list in storage.json), else the main checkout, so that a worktree's session finds its checkout's window."""
    here = Path(cwd).resolve()
    try:
        state = json.loads((STORAGE / 'Code/User/globalStorage/storage.json').read_text(encoding='utf-8'))['windowsState']
        uris = [w.get('folder') for w in [state.get('lastActiveWindow') or {}, *state.get('openedWindows', [])]]
        held = [f for f in (Path(unquote(urlparse(u).path)) for u in uris if isinstance(u, str) and u.startswith('file://'))
                if f == here or f in here.parents]
    except (OSError, ValueError, KeyError, TypeError, AttributeError):
        held = []
    if held:
        return max(held, key=lambda f: len(f.parts))
    common = git(cwd, 'rev-parse', '--path-format=absolute', '--git-common-dir')
    return Path(common).parent if Path(common).name == '.git' else Path(git(cwd, 'rev-parse', '--show-toplevel') or here)


def link(folder):
    """The vscode:// URI a Windows toast opens for folder, or None on WSL without a distro name."""
    if SYSTEM == 'wsl':
        distro = os.environ.get('WSL_DISTRO_NAME')
        return f'vscode://vscode-remote/wsl+{quote(distro)}{quote(folder.as_posix())}' if distro else None
    return 'vscode://file' + quote(('' if folder.as_posix().startswith('/') else '/') + folder.as_posix(), safe='/:')


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


def command(header, body, cwd, focus):
    """The notifier's argv on macOS, Windows or WSL, a click focusing cwd's window when focus; None without a notifier."""
    if SYSTEM == 'darwin':
        if tn := shutil.which('terminal-notifier'):
            click = ['-execute', f'open -b com.microsoft.VSCode {shlex.quote(str(window(cwd)))}'] if focus else []
            return [tn, '-title', header, '-message', body, '-group', header, *click]
        osa = shutil.which('osascript')
        return osa and [osa, '-e', 'on run argv', '-e', 'display notification (item 2 of argv) with title (item 1 of argv)',
                        '-e', 'end run', header, body]
    ps = shutil.which('powershell.exe') or shutil.which('powershell') or (str(WIN_PS) if SYSTEM == 'wsl' and WIN_PS.exists() else None)
    return ps and [ps, *toast(header, body, link(window(cwd)) if focus else None)]


def click(send, code, folder, token_fd, header, body):
    """Detached helper on Linux: a notification with a default action; a click within WAIT seconds runs `code folder`
    with the activation token the daemon hands over (when this notify-send can), which Wayland needs to raise a window.
    Variables naming the Claude Code session are dropped, since a window `code` opens takes its environment."""
    r, w = os.pipe()
    p = subprocess.Popen([send, APP, '--action=default=Open', *([f'--activation-token-fd={w}'] if token_fd == '1' else []), header, body],
                         pass_fds=(w,), stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
    os.close(w)
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
    with os.fdopen(r, 'rb') as f:
        token = f.read(4096).decode('utf-8', 'replace').strip()
    if 'default' in out.split():
        env = {k: v for k, v in os.environ.items() if not DROP.fullmatch(k)}
        env |= {'XDG_ACTIVATION_TOKEN': token, 'DESKTOP_STARTUP_ID': token} if token else {}
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
    focus = os.environ.get('TERM_PROGRAM') == 'vscode'
    if SYSTEM in ('darwin', 'win32', 'wsl'):
        argv = command(header, body, cwd, focus)
    elif not (send := shutil.which('notify-send')):
        return
    else:
        argv = [send, APP, header, body]
        if focus and (code := shutil.which('code')):
            options = subprocess.run([send, '--help'], capture_output=True, text=True, timeout=left(2)).stdout
            if '--action' in options:  # libnotify 0.7.10; the token fd needs 0.8.8
                subprocess.Popen([sys.executable, __file__, '--click', send, code, str(window(cwd)),
                                  str(int('--activation-token-fd' in options)), header, body], start_new_session=True,
                                 stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                return
    if argv:
        subprocess.run(argv, capture_output=True, timeout=left(8))


if __name__ == '__main__':
    try:
        click(*sys.argv[2:8]) if sys.argv[1:2] == ['--click'] else main()
    except Exception:  # A notification must never disturb the session.
        pass
    sys.exit(0)
