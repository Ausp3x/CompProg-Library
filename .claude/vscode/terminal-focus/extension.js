'use strict';
// <urlProtocol>://compprog.terminal-focus/focus?pid=<pid>[&folder=<file URI>] shows the integrated terminal of this window
// whose shell is pid or its nearest ancestor (.claude/hooks/notify.py passes the terminal's shell, else the hook's parent).
// VS Code hands the URI to the window focused last; when that window lacks the terminal and a folder is given, it leaves
// the request in the extension's global storage and, once VS Code has focused it, opens the folder, which focuses the
// window holding it, and that window shows the terminal as it gains focus. Ancestors come from /proc, else `ps` (macOS); elsewhere only the pid
// itself matches. No dependencies and no build step.
const vscode = require('vscode');
const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');
const FRESH = 15000;  // ms a left request stays valid: the folder's window gains focus well within it
const WAIT = 2000;  // ms the relay watches this window's focus after taking a URI

function parent(pid) {
    try {
        const s = fs.readFileSync(`/proc/${pid}/stat`, 'utf8');
        return Number(s.slice(s.lastIndexOf(')') + 2).split(' ')[1]);
    } catch {
        try {
            return Number(execFileSync('ps', ['-o', 'ppid=', '-p', String(pid)], { encoding: 'utf8', timeout: 2000 }).trim());
        } catch {
            return 0;
        }
    }
}

async function show(pid) {
    const shells = new Map();
    for (const t of vscode.window.terminals) {
        const id = await t.processId;
        if (id) shells.set(id, t);
    }
    for (let hops = 0; pid > 1 && hops < 64; hops++, pid = parent(pid)) {
        if (shells.has(pid)) {
            shells.get(pid).show();
            return true;
        }
    }
    return false;
}

const request = context => path.join(context.globalStorageUri.fsPath, 'request.json');

async function focus(uri, context) {
    // uri.query arrives decoded once, the folder's URI as notify.py quoted it, so it is taken without a second decode
    const pid = Number(new URLSearchParams(uri.query).get('pid')), folder = (/(?:^|&)folder=([^&]*)/.exec(uri.query) || [])[1];
    if (uri.path !== '/focus' || !Number.isInteger(pid)) return false;
    if (await show(pid)) return true;
    if (!folder || !context) return false;
    try {
        fs.mkdirSync(context.globalStorageUri.fsPath, { recursive: true });
        fs.writeFileSync(request(context), JSON.stringify({ pid, at: Date.now() }));
    } catch {
        return false;
    }
    afterFocus(() => vscode.commands.executeCommand('vscode.openFolder', vscode.Uri.parse(folder), { forceNewWindow: true }));
    return false;
}

// VS Code focuses the window that handled a URI once the handler returns (URLService.handleURL, 1.140), so the folder
// must open after that. An unfocused window opens it when it gains focus, else after WAIT. A focused one gets no such
// event, so it opens it at once and once more if, having lost focus to the folder's window, it regains it within WAIT.
function afterFocus(open) {
    const focused = vscode.window.state.focused;
    let timer, sub, away = !focused;
    const once = () => { clearTimeout(timer); sub.dispose(); open(); };
    if (focused) open();
    sub = vscode.window.onDidChangeWindowState(state => state.focused ? away && once() : (away = true));
    timer = setTimeout(focused ? () => sub.dispose() : once, WAIT);
}

async function pending(context) {
    try {
        const { pid, at } = JSON.parse(fs.readFileSync(request(context), 'utf8'));
        if (Date.now() - at <= FRESH && await show(pid)) fs.unlinkSync(request(context));
    } catch {
        // no request, or one another window took
    }
}

exports.focus = focus;
exports.activate = context => context.subscriptions.push(
    vscode.window.registerUriHandler({ handleUri: uri => focus(uri, context) }),
    vscode.window.onDidChangeWindowState(state => state.focused && pending(context)));
exports.deactivate = () => {};
