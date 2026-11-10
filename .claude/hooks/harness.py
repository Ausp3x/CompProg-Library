#!/usr/bin/env python3
"""Harness guard: the experiment id, server model mappings, the served model and the request settings of a Claude Code
session, read from the client's own files (evidence and limits: 00-Guidelines/13-Plan/Docs/00-harness-audit.md).

  harness.py [--session ID | --transcript PATH]   report for the session (default: $CLAUDE_CODE_SESSION_ID)
  harness.py --experiment                          the session's experiment id and key, `none` or `unknown`
  harness.py --selftest                            exit 1 unless the guard is active: hooks registered in the effective
                                                   settings, none disabled, a guard heartbeat in the last two minutes
  harness.py hook                                  hook entry, JSON on stdin (see hook())

Read-only and never on the network: it reads file tails only, writes nothing but its own state under
.claude/hooks/.state/harness/ (heartbeats, Stop records, the session's warned, hide-text and /effort lists, the newest
subagent replies with those refused, errors.log), never opens .credentials.json, and prints no token, account or
organization id, e-mail address or message text. The allowed models are read at every check: `models` in packages.json, the `model:` of the
agent files and skill front matter, and `models` in .claude/harness.json. Detectors match by shape (model ids, hex
experiment ids, objects mapping model ids to model ids); nothing here names a model, effort, flag or experiment."""
import argparse
import hashlib
import json
import os
import re
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main_checkout(root):
    """The main checkout of root: a linked worktree's `.git` file names its git dir, whose `commondir` leads to the
    main repository's; any other layout is its own main checkout."""
    try:
        git = (root / '.git').read_text().strip()
        d = (root / git.removeprefix('gitdir:').strip()).resolve()
        common = (d / (d / 'commondir').read_text().strip()).resolve()
        return common.parent if common.name == '.git' else root  # a bare repository has no main checkout
    except (OSError, ValueError):
        return root


STATE = main_checkout(ROOT) / '.claude/hooks/.state/harness'  # one state for every checkout: hooks and --selftest agree
CONF = ROOT / '.claude/harness.json'
TAIL = 1 << 20
DEEP = 16 << 20  # read this far back when a record over TAIL (a large tool result) hides the newest reply
FRESH = 120  # seconds: the newest heartbeat --selftest accepts
MODEL = re.compile(r'claude-(?:(?:[a-z]+-)+\d{1,2}(?:-\d{1,2})*|\d{1,2}(?:-\d{1,2})*(?:-[a-z]+)+)(?:-\d{8})?')
HEX = re.compile(r'[0-9a-f]{8,64}')
NOT = r"(?:do\s+not|don['’]?t|never|must\s+not|should\s+not)"
HIDE = re.compile(rf"\b{NOT}\s+(?:mention|reveal|disclose|share|bring\s+up)\b[^.\n]{{0,60}}?\b(?:to|with)\s+the\s+user\b"
                  rf"|\b{NOT}\s+(?:tell|inform|notify|alert|let)\s+the\s+user\b(?!\s+(?:to|how)\b)"
                  r"|\bwithout\s+(?:telling|informing|notifying|alerting|letting)\s+the\s+user\b"
                  r"|\b(?:hide|conceal|withhold|keep)\b[^.\n]{0,40}?\bfrom\s+the\s+user\b"
                  r"|\bthe\s+user\s+(?:must|should)\s+(?:not|never)\s+(?:know|see|learn|find\s+out|be\s+told)\b", re.I)
SECRET = re.compile(r'[\w.+-]+@[\w-]+(?:\.[\w-]+)+|\b[0-9a-f]{8}-(?:[0-9a-f]{4}-){3}[0-9a-f]{12}\b|\bsk-[\w-]{8,}'
                    r'|\b(?:msg|req|toolu|srvtoolu)_\w{8,}', re.I)
CTRL = re.compile(r'[\x00-\x1f\x7f-\x9f\u2028\u2029]')
SOURCE_KEYS = {'path', 'displayPath', 'filename', 'filePath', 'file_path', 'outputFilePath', 'file', 'files', 'prompt',
               'hookName', 'hookEvent'}  # an attachment holding a file, a typed prompt or a local hook's output
MANAGED = ('/etc/claude-code', '/Library/Application Support/ClaudeCode', 'C:/Program Files/ClaudeCode', 'C:/ProgramData/ClaudeCode')
HOW = ('To allow a model, name it in `models` of 00-Guidelines/13-Plan/packages.json, in the agent files or skill front '
       'matter that use it, or in `models` of .claude/harness.json (CLAUDE.md, Models and delegation).')
UNDO = ('/rewind to the message before that reply restores the files its Edit, Write and NotebookEdit calls changed; '
        'git status and git diff show what its Bash calls changed.')
REAUDIT = 'package P287, 00-Guidelines/13-Plan/Docs/00-harness-audit.md'
ORDER = ('map', 'model', 'setting', 'blind', 'idle', 'hide', 'hooks', 'client')


def redact(text):
    """text with secrets replaced and control characters turned into spaces, so an output line stays one line."""
    return CTRL.sub(' ', SECRET.sub('<redacted>', str(text)))


def cut(v, n=40):
    """v redacted, then cut to n characters: cutting first could leave part of a secret unmatched."""
    return redact(v)[:n]


def obj(v):
    return v if isinstance(v, dict) else {}


def load(path):
    """The JSON object in path, or {} when it is absent, unreadable or not an object."""
    try:
        v = json.loads(Path(path).read_text(errors='replace'))
    except (OSError, ValueError, TypeError, RecursionError):
        return {}
    return v if isinstance(v, dict) else {}


def tail(path, size=TAIL):
    """The JSON objects on the last `size` bytes of a JSONL file, oldest first; a cut first line and broken lines are skipped."""
    if not isinstance(path, (str, Path)):
        return []
    try:
        with open(path, 'rb') as f:
            end = f.seek(0, 2)
            f.seek(max(0, end - size))
            lines = f.read().split(b'\n')[end > size:]
    except (OSError, TypeError, ValueError):
        return []
    out = []
    for line in lines:
        try:
            e = json.loads(line)
        except (ValueError, RecursionError):  # deeper nesting than Python parses: skipped like a broken line
            continue
        if isinstance(e, dict):
            out.append(e)
    return out


def strings(v, depth=0):
    if isinstance(v, str):
        yield v
    elif depth < 12 and isinstance(v, (dict, list)):
        for x in v.values() if isinstance(v, dict) else v:
            yield from strings(x, depth + 1)


def base(v):
    """A model string without a context suffix such as [1m]."""
    return re.sub(r'\[[^\]]*\]$', '', v) if isinstance(v, str) else v


def undated(v):
    """A model id without its context suffix and snapshot date."""
    return re.sub(r'-\d{8}$', '', base(v)) if isinstance(v, str) else v


def is_model(v):
    return isinstance(v, str) and MODEL.fullmatch(base(v)) is not None


def short(v):
    return cut(str(v).removeprefix('claude-'), 24)


def vtuple(v):
    return tuple(int(x) for x in re.findall(r'\d+', str(v))[:4])


def config_dir():
    d = os.environ.get('CLAUDE_CONFIG_DIR')
    return Path(d).expanduser() if d else Path.home() / '.claude'


def global_config():
    """The client's global config file, by the rule of the audited release: a legacy .config.json in the config folder
    when it is a non-empty file, or an empty writable one while the new file is missing; else .claude.json
    (.claude-custom-oauth.json with CLAUDE_CODE_CUSTOM_OAUTH_URL) in $CLAUDE_CONFIG_DIR when it is set, else in the
    home folder."""
    d = os.environ.get('CLAUDE_CONFIG_DIR')
    new = (Path(d).expanduser() if d else Path.home()) / ('.claude-custom-oauth.json' if os.environ.get('CLAUDE_CODE_CUSTOM_OAUTH_URL') else '.claude.json')
    old = config_dir() / '.config.json'
    try:
        st = old.stat()
        if old.is_file() and (st.st_size > 0 or st.st_mode & 0o222 and not new.exists()):
            return old
    except OSError:
        pass
    return new


def front(path):
    """The key: value lines of a Markdown file's front matter."""
    try:
        m = re.match(r'---\r?\n(.*?)\r?\n---', Path(path).read_text(errors='replace')[:8192], re.S)
    except OSError:
        return {}
    return {k: v.strip().strip('"\'') for k, v in re.findall(r'^([\w-]+):[ \t]*(.*)$', m.group(1), re.M)} if m else {}


def allowed(root=ROOT):
    """(allowed model ids, problems) from packages.json, the agent files and skill front matter, whose values must
    have the model-id shape (an alias is a problem), and from harness.json, whose entries count as exact ids."""
    ids, bad = set(), []

    def add(v, where):
        if is_model(v):
            ids.add(base(v))
        elif v not in (None, '', 'inherit'):
            bad.append(f'{where} names model {cut(v)!r}, not a model id')
    models = load(root / '00-Guidelines/13-Plan/packages.json').get('models')
    for v in models.values() if isinstance(models, dict) else ():
        add(v, 'packages.json')
    for p in sorted((root / '.claude/agents').glob('*.md')) + sorted((root / '.claude/skills').glob('*/SKILL.md')):
        add(front(p).get('model'), p.relative_to(root).as_posix())
    extra = load(root / '.claude/harness.json').get('models')
    for v in extra if isinstance(extra, list) else ():  # exact ids, any shape: provider ids such as Bedrock's included
        if isinstance(v, str) and v.strip():
            ids.add(base(v.strip()))
    return ids, bad


def safe(v):
    return re.sub(r'[^A-Za-z0-9_-]', '', str(v or ''))[:100]


def transcript(hook=None, session=None, path=None):
    """The transcript to check: a subagent's own when the hook fires inside one, else the session's."""
    if path:
        return Path(path)
    hook = {k: v for k, v in obj(hook).items() if isinstance(v, str) and v}
    if hook.get('agent_transcript_path'):
        return Path(hook['agent_transcript_path'])
    tp = hook.get('transcript_path')
    if tp and hook.get('agent_id'):
        sub = Path(tp).with_suffix('') / 'subagents' / f'agent-{safe(hook["agent_id"])}.jsonl'
        if sub.is_file():
            return sub
    if tp:
        return Path(tp)
    sid = safe(session or os.environ.get('CLAUDE_CODE_SESSION_ID'))
    found = sorted((config_dir() / 'projects').glob(f'*/{sid}.jsonl'), key=lambda p: p.stat().st_mtime) if sid else []
    return found[-1] if found else None


def main_of(path):
    """The main transcript of a subagent transcript (<session>/subagents/agent-*.jsonl), else path itself."""
    p = Path(path)
    return p.parent.parent.with_suffix('.jsonl') if p.parent.name == 'subagents' else p


def newest_reply(recs):
    """The newest assistant record that is not a synthetic error reply, or None."""
    for e in reversed(recs):
        m = e.get('message') if e.get('type') == 'assistant' else None
        if isinstance(m, dict) and m.get('model') != '<synthetic>':
            return e
    return None


def requested(recs, reply):
    """The reply's requested model: `requestedModel`, else the newest model attachment before it, else None."""
    if isinstance(reply.get('requestedModel'), str) and reply['requestedModel'] != '<synthetic>':
        return reply['requestedModel']
    for e in reversed(recs[:next((i for i, x in enumerate(recs) if x is reply), len(recs))]):
        a = e.get('attachment') if e.get('type') == 'attachment' else None
        mid = (a.get('identity') or {}).get('modelId') if isinstance(a, dict) and a.get('type') == 'model' and isinstance(a.get('identity'), dict) else None
        if isinstance(mid, str) and mid != '<synthetic>':
            return mid
    return None


def model_issues(recs, ids):
    """[(kind, short, long)] for the newest reply (blind fields, a served model that differs from the requested one,
    either outside the allowed set) and for the newest request, whose reply is not on disk while it is in flight. A
    model value that is not a string is outside the set."""
    out, reply = [], newest_reply(recs)

    def off(label, v):
        if v is not None and not (isinstance(v, str) and base(v) in ids):
            shape = '' if is_model(v) else ' (unknown shape)'
            out.append(('model', f'off-list {short(v)}', f'{label} {cut(v)}{shape} is outside the allowed set {", ".join(sorted(ids)) or "(empty)"}'))
    if reply:
        served, want = reply['message'].get('model'), requested(recs, reply)
        if served is None:
            out.append(('blind', 'blind served', 'the newest reply has no message.model'))
        if want is None:
            out.append(('blind', 'blind requested', 'the newest reply has no requestedModel and no model attachment precedes it'))
        if served is not None and want is not None and base(served) != base(want) and not (
                undated(served) == undated(want) and base(served) in ids):  # a snapshot the allowed set names
            out.append(('model', f'{short(served)}≠{short(want)}', f'served by {cut(served)}, requested {cut(want)}'))
        off('served model', served)
        off('requested model', want)
    off('the newest request model', next((e['params'].get('model') for e in reversed(recs)
                                          if e.get('type') == 'api-request' and isinstance(e.get('params'), dict)), None))
    return out


def slots(gc):
    """The experiment cache slots of the global config, newest first."""
    s = gc.get('clientDataCacheSlots')
    vals = s.values() if isinstance(s, dict) else s if isinstance(s, list) else ()
    out = [v for v in vals if isinstance(v, dict) and isinstance(v.get('data'), dict)]
    return sorted(out, key=lambda v: v['at'] if isinstance(v.get('at'), (int, float)) else 0, reverse=True)


def slot_format(gc):
    """[problem]: how the global config departs from the experiment cache format the SUP28 audit found, which launch.py
    and the slot checks rely on: clientDataCacheSlots, an object of {at, model, entrypoint, data} slots whose data holds
    an optional hex `atis` and string `experimentKey`."""
    s = gc.get('clientDataCacheSlots')
    if s is None:  # no slots yet; inspect reports a latch id that no slot can hold (a renamed container)
        return []
    if not isinstance(s, dict):
        return ['clientDataCacheSlots is not an object']
    out = set()
    for v in s.values():
        if not (isinstance(v, dict) and isinstance(v.get('data'), dict) and isinstance(v.get('model'), str)
                and isinstance(v.get('entrypoint'), str) and isinstance(v.get('at'), (int, float))):
            out.add('a slot lacks at, model, entrypoint or data')
            continue
        a, k = v['data'].get('atis'), v['data'].get('experimentKey')
        if not (a is None or isinstance(a, str) and (not a or HEX.fullmatch(a))):
            out.add('a slot atis is not a hex string')
        if not (k is None or isinstance(k, str)):
            out.add('a slot experimentKey is not a string')
    return sorted(out)


def slot_for(ss, model, entry):
    return next((v for v in ss if base(v.get('model')) == base(model) and v.get('entrypoint') == entry), None)


def experiment(main_recs, ss, model, entry):
    """(id or None, key or None, source, [problem]) from the newest atis-latch record, else the newest slot of the
    model and entrypoint. An empty latch means no experiment."""
    for e in reversed(main_recs):
        if e.get('type') == 'atis-latch':
            v = e.get('atis')
            if not isinstance(v, str):
                return None, None, 'blind', ['an atis-latch record has no atis']
            if v and not HEX.fullmatch(v):
                return None, None, 'unknown', ['an atis-latch value has an unknown shape']
            key = next((s['data'].get('experimentKey') for s in ss if v and s['data'].get('atis') == v), None)
            return v or None, key if isinstance(key, str) else None, 'latch', []
    s = slot_for(ss, model, entry)
    if s is None:
        return None, None, 'none', ['no atis-latch record in the transcript tail and no cached slot for the model']
    v, key = s['data'].get('atis'), s['data'].get('experimentKey')
    if not (v is None or isinstance(v, str) and (not v or HEX.fullmatch(v))):
        return None, None, 'unknown', ['a slot atis has an unknown shape']
    return v or None, key if isinstance(key, str) else None, 'slot', []


def mappings(slot):
    """([(path, from, to)], [unknown-shape note]): every object in the slot whose keys and values are model ids, and
    objects keyed by model ids with other values."""
    found, odd = [], []

    def walk(v, path, depth):
        if isinstance(v, dict) and v and depth < 8:
            if all(is_model(k) for k in v):
                if all(is_model(x) for x in v.values()):
                    found.extend((path, k, x) for k, x in v.items())
                    return
                odd.append(f'{path or "slot"}: keyed by model ids with other values')
            for k, x in v.items():
                walk(x, f'{path}.{k}'.lstrip('.'), depth + 1)
        elif isinstance(v, list) and depth < 8:
            for x in v:
                walk(x, path + '[]', depth + 1)
    walk((slot or {}).get('data'), '', 0)
    return found, odd


def subset(want, have):
    if isinstance(want, dict):
        return isinstance(have, dict) and all(k in have and subset(x, have[k]) for k, x in want.items())
    if isinstance(want, list):
        return isinstance(have, list) and all(any(subset(w, h) for h in have) for w in want)
    return want == have


def find_key(v, key, depth=0):
    if isinstance(v, dict) and depth < 6:
        for k, x in v.items():
            if k == key and isinstance(x, str):
                yield x
            yield from find_key(x, key, depth + 1)


def launch_effort():
    """The --effort of the Claude Code process ($CLAUDE_PID) where /proc shows its command line, else None."""
    try:
        argv = Path(f'/proc/{int(os.environ["CLAUDE_PID"])}/cmdline').read_bytes().decode(errors='replace').split('\0')
    except (KeyError, ValueError, OSError):
        return None
    for i, a in enumerate(argv):
        if a == '--effort' and i + 1 < len(argv):
            return argv[i + 1]
        if a.startswith('--effort='):
            return a.split('=', 1)[1]
    return None


def chosen_efforts(recs, main_recs, reply, model, meta, live, sid=None, root=ROOT):
    """The efforts the user chose for this thread, as a union: a subagent's own (meta.json or its agent file) alone,
    else the launch flag, every /effort command of the session (remembered once seen, so none scrolls out of the
    tail), the effort settings for the model, and a per-turn change (a skill's front matter, or a reply whose per-turn
    effort differs from the session effort). The session agent's front matter is no source: the client ignores it for
    a main session (CLAUDE.md, Models and delegation). Empty when none was chosen explicitly. The launch flag comes
    from this process, so it is read only for the live session (`live`); for any other transcript the session effort
    the reply records counts as chosen instead."""
    out = []
    agents = [root / '.claude/agents', config_dir() / 'agents']
    own = meta.get('effort') or next((front(d / f'{safe(meta.get("agentType"))}.md').get('effort') for d in agents
                                      if meta.get('agentType') and (d / f'{safe(meta["agentType"])}.md').is_file()), None)
    if isinstance(own, str) and own:
        return {own}
    out.append(launch_effort() if live else (reply or {}).get('effort'))
    seen = {}
    for e in main_recs:
        c = obj(e.get('message')).get('content') if e.get('type') == 'user' else None
        if isinstance(c, str):
            seen.update((a.split()[0], 1) for a in re.findall(r'<command-name>/effort</command-name>.*?<command-args>([^<]*)</command-args>', c, re.S) if a.split())
    out += list(remember(sid, 'effort', seen) if sid else seen)
    for f in (config_dir() / 'settings.json', root / '.claude/settings.json', root / '.claude/settings.local.json'):
        s = load(f)
        per = s.get('modelSettings') if isinstance(s.get('modelSettings'), dict) else {}
        out += [s.get('effortLevel'), obj(per.get(model)).get('effortLevel') if isinstance(model, str) else None]
    skill = reply.get('attributionSkill') if reply else None
    if isinstance(skill, str):
        out.append(front(root / '.claude/skills' / safe(skill) / 'SKILL.md').get('effort'))
    if reply and reply.get('perTurnEffort') != reply.get('effort'):
        out.append(reply.get('perTurnEffort'))
    return {x for x in out if isinstance(x, str) and x}


def request_issues(recs, main_recs, reply, model, meta, conf, live, sid=None):
    """[(kind, short, long)] for the newest request of the thread against the chosen settings (deviations only warn)."""
    reqs = [e for e in recs if e.get('type') == 'api-request' and isinstance(e.get('params'), dict)]
    if not reqs:
        return [('blind', 'blind request', 'no api-request record in the transcript tail')] if reply else []
    r = next((e for e in reversed(reqs) if reply and e.get('id') == reply.get('requestRef')), reqs[-1])
    p, out, spec = r['params'], [], conf.get('request') if isinstance(conf.get('request'), dict) else {}
    for k in ('model', 'max_tokens', 'thinking', 'context_management'):
        if k not in p:
            out.append(('blind', f'blind {k}', f'the newest request has no {k}'))
    efforts = list(find_key(p, 'effort'))
    chosen = chosen_efforts(recs, main_recs, reply, model, meta, live, sid)
    if not efforts:
        out.append(('blind', 'blind effort', 'the newest request has no effort'))
    elif chosen and any(e not in chosen for e in efforts):
        out.append(('setting', f'effort {efforts[0]}', f'request effort {efforts[0]}, chosen {", ".join(sorted(chosen))}'))
    for k, want in (spec.get('expect') or {}).items():
        if k in p and not subset(want, p[k]):
            out.append(('setting', k, f'request {k} differs from the audited {json.dumps(want)}'))
    limit = os.environ.get('CLAUDE_CODE_MAX_OUTPUT_TOKENS') if live else None
    if limit and limit.isascii() and limit.isdigit() and p.get('max_tokens') != int(limit):
        out.append(('setting', 'max_tokens', f'request max_tokens {p.get("max_tokens")}, chosen {limit}'))
    same = [e['params'] for e in reqs if e.get('querySource') == r.get('querySource') and e['params'].get('model') == p.get('model')]
    for k in ('max_tokens', 'thinking', 'context_management'):
        if len({json.dumps(q.get(k), sort_keys=True) for q in same}) > 1:
            out.append(('setting', f'{k} changed', f'{k} changed between the requests in the transcript tail'))
    known = set(spec.get('fields') or ())
    for k, v in p.items():
        if k != 'model' and any(is_model(s) for s in strings(v)):
            out.append(('setting', k, f'request field {k} names a model (a fallback the user never chose)'))
        elif known and k not in known:
            out.append(('setting', cut(f'new {k}', 20), f'request field {k} is new since the audit'))
    return out


def sentence(text, m, span=200):
    """The sentence around match m, or the match alone when the sentence reaches further than span characters to
    either side (which keeps a long text without full stops linear), lowercased with digits and spacing normalized."""
    lo, hi = m.start() - span, m.end() + span
    a = max(text.rfind('.', max(lo, 0), m.start()), text.rfind('\n', max(lo, 0), m.start())) + 1
    ends = [i for i in (text.find('.', m.end(), hi), text.find('\n', m.end(), hi)) if i >= 0]
    b = min(ends) if ends else len(text) if hi >= len(text) else None
    if b is None or a == 0 and lo > 0:
        a, b = m.start(), m.end()
    return re.sub(r'\s+', ' ', re.sub(r'\d+', '0', text[a:b])).strip().lower()


def parts(a):
    """The attachment and the objects directly inside it, in lists too."""
    for x in [a, *a.values()]:
        for d in x if isinstance(x, list) else [x]:
            if isinstance(d, dict):
                yield d


def hide_texts(recs, slot):
    """{hash: where} for injected or server-delivered text that tells the model to hide something from the user:
    attachments without user or file content, system records, and the slot's strings."""
    found = {}

    def scan(where, texts):
        for s in texts:
            for _, m in zip(range(20), HIDE.finditer(s)):  # the first 20 matches of a text
                found.setdefault(hashlib.sha256(sentence(s, m).encode()).hexdigest()[:12], where)
    for e in recs:
        a = e.get('attachment') if e.get('type') == 'attachment' else None
        if isinstance(a, dict) and not any(k in SOURCE_KEYS for d in parts(a) for k in d):
            scan(f'attachment {cut(a.get("type"))}', [*strings(a), *strings(e.get('rendered'))])
        elif e.get('type') == 'system':
            scan(f'system {cut(e.get("subtype"))}', strings(e.get('content')))
    for k, v in ((slot or {}).get('data') or {}).items():
        scan(f'slot {cut(k)}', strings(v))
    return found


def layers(root=ROOT):
    """[(label, settings)] for every settings file that applies, managed and server-delivered policy first."""
    out = []
    for d in (Path(m) for m in MANAGED if Path(m).is_absolute()):  # only this platform's folders
        for f in [d / 'managed-settings.json', *sorted((d / 'managed-settings.d').glob('*.json'))]:
            if f.is_file():
                out.append((f'managed {f}', load(f)))
    for name in ('remote-settings.json', 'policy-limits.json'):
        if (config_dir() / name).is_file():
            out.append((f'managed {name}', load(config_dir() / name)))
    return out + [('user settings', load(config_dir() / 'settings.json')), ('project settings', load(root / '.claude/settings.json')),
                  ('local settings', load(root / '.claude/settings.local.json'))]


def hooks_off(ls):
    """[text] for settings that could disable hooks: a true managed key naming hooks, or a true disable key anywhere."""
    return [f'{label}: {k}' for label, s in ls for k, v in s.items()
            if 'hook' in k.lower() and v is True and (label.startswith('managed') or 'disable' in k.lower())]


def registered(ls, event, every=False):
    """True when some layer runs `harness.py hook` on event, for every tool when `every`."""
    for _, s in ls:
        groups = (s.get('hooks') or {}).get(event) if isinstance(s.get('hooks'), dict) else None
        for g in groups if isinstance(groups, list) else ():
            if isinstance(g, dict) and (not every or g.get('matcher') in (None, '', '*')):
                if any(isinstance(h, dict) and re.search(r'harness\.py["\']?\s+hook\b', str(h.get('command'))) for h in g.get('hooks') or ()):
                    return True
    return False


def finished_tool_use(recs):
    """The id of the newest tool call that already has its result in the records, or None. Its PreToolUse hook ran
    before the tool did, so its heartbeat must exist."""
    done = set()
    for e in reversed(recs):
        c = obj(e.get('message')).get('content')
        for b in c if isinstance(c, list) else ():
            i = b.get('tool_use_id' if e.get('type') == 'user' else 'id') if isinstance(b, dict) else None
            if not isinstance(i, str):
                continue
            if b.get('type') == 'tool_result' and e.get('type') == 'user':
                done.add(i)
            elif b.get('type') == 'tool_use' and e.get('type') == 'assistant' and i in done:
                return i
    return None


def beat(session, tool_use_id, info):
    """Store the check of tool_use_id as its marker and as last.json; prune old markers and sessions."""
    d = STATE / (safe(session) or 'session')
    d.mkdir(parents=True, exist_ok=True)
    (d / (safe(tool_use_id) or 'call')).write_text(json.dumps(info))
    tmp = d / f'.last.{os.getpid()}'
    tmp.write_text(json.dumps(info))
    os.replace(tmp, d / 'last.json')
    marks = [p for p in d.iterdir() if '.' not in p.name]  # call markers: safe() names, unlike the .json state
    if len(marks) > 300:
        for p in sorted(marks, key=lambda p: p.stat().st_mtime)[:100]:
            p.unlink(missing_ok=True)
    if hash(str(tool_use_id)) % 50 == 0:
        for s in STATE.iterdir():
            if s.is_dir() and s.stat().st_mtime < time.time() - 7 * 86400:
                for p in s.iterdir():
                    p.unlink(missing_ok=True)
                s.rmdir()


def remember(session, name, found=None):
    """The sightings stored for the session under name (`hide`: hide-text hashes, `effort`: /effort commands), after
    adding `found`; the first sighting of a key is kept."""
    f = STATE / (safe(session) or 'session') / f'{name}.json'
    old = load(f)
    if found and not set(found) <= set(old):
        old.update({k: v for k, v in found.items() if k not in old})
        f.parent.mkdir(parents=True, exist_ok=True)
        tmp = f.with_name(f'.{name}.{os.getpid()}')
        tmp.write_text(json.dumps(old))
        os.replace(tmp, f)
    return old


def inspect(path, session=None, model=None, entry=None, version=None, live=None, root=ROOT, guard=False, subagents=False):
    """Every check for one transcript: {'exp': (id, key, source), 'issues': [(kind, short, long)], 'notes': [text],
    'ids': allowed set, 'hide': {hash: where}, ...}. `live` (default: the transcript is $CLAUDE_CODE_SESSION_ID's)
    lets the checks use this process's environment: the entrypoint, the launch effort and the output-token limit.
    `guard` returns right after a failed model check, so no slower check can delay a deny past the hook timeout.
    `subagents` (main-thread hooks and the status line of the live session) adds the subagent check."""
    path = Path(path) if isinstance(path, (str, Path)) and str(path) else None
    session = session if isinstance(session, str) else None
    model = model if isinstance(model, str) else None
    version = version if isinstance(version, str) else None
    recs = tail(path) if path else []
    try:
        size = path.stat().st_size if path else 0
    except OSError:
        size = 0
    if size > TAIL and newest_reply(recs) is None:
        recs = tail(path, DEEP)
    main = main_of(path) if path else None
    main_recs = recs if main == path else tail(main)
    meta = load(path.with_suffix('.meta.json')) if path and main != path else {}
    ids, notes = allowed(root)
    reply = newest_reply(recs)
    if reply:
        model = requested(recs, reply) or model
    sid = session or next((e['sessionId'] for e in reversed(main_recs) if isinstance(e.get('sessionId'), str)), None)
    live = live if live is not None else bool(sid) and sid == os.environ.get('CLAUDE_CODE_SESSION_ID')
    issues, subkeys = model_issues(recs, ids), []  # first and on their own: no other check's failure may switch the guard off
    if subagents and live and path and main == path:
        try:
            more, subkeys = subagent_issues(path, sid, ids)
            issues += more
        except Exception as exc:
            log_error(exc)
            issues.append(('blind', 'blind subagents', f'the subagent check failed ({type(exc).__name__})'))
    if reply is None and size > DEEP:
        issues.append(('blind', 'blind reply', f'no reply in the last {DEEP >> 20} MiB of the transcript'))
    exp, maps, hide = (None, None, 'unknown'), [], {}
    if guard and any(k == 'model' for k, _, _ in issues):
        return {'exp': exp, 'issues': issues, 'notes': notes, 'ids': ids, 'hide': hide, 'maps': maps, 'model': model,
                'entry': entry, 'version': version, 'recs': recs, 'reply': reply, 'sid': sid, 'subkeys': subkeys}
    try:
        conf = load(root / '.claude/harness.json')
        entry = entry or (os.environ.get('CLAUDE_CODE_ENTRYPOINT') if live else None) or next((e['entrypoint'] for e in reversed(main_recs) if isinstance(e.get('entrypoint'), str)), None)
        version = version or next((e['version'] for e in reversed(recs) if isinstance(e.get('version'), str)), None)
        gc_path = global_config()
        try:
            raw = json.loads(gc_path.read_text(errors='replace')) if gc_path.is_file() else {}
        except (OSError, ValueError, RecursionError):
            raw = None
        gc = raw if isinstance(raw, dict) else {}
        ss = slots(gc)
        exp = experiment(main_recs, ss, model, entry)
        drift = (slot_format(gc) if isinstance(raw, dict) else ['the global config is not a JSON object']) if gc_path.is_file() else []
        if gc_path.is_file() and 'clientDataCacheSlots' not in gc and exp[2] == 'latch' and exp[0]:
            drift.append('the latch shows an experiment id but the global config has no clientDataCacheSlots')
        if drift and exp[2] in ('slot', 'none'):  # without a latch the id would come from a slot of unknown format
            exp = (None, None, 'unknown', [])
        slot = slot_for(ss, model, entry) or (ss[0] if ss else None)
        maps, odd = mappings(slot)
        issues += [('map', f'map {short(a)}→{short(b)}', f'slot {p} maps {a} to {b}') for p, a, b in maps if base(model) == base(a)]
        issues += request_issues(recs, main_recs, reply, model, meta, conf, live, sid)
        issues += [('blind', 'blind experiment', t) for t in exp[3]] if path and exp[2] in ('blind', 'unknown') else []
        notes += exp[3] if exp[2] == 'none' else []
        issues += [('blind', 'slots format', f'{t}: launch.py strips nothing and the slot checks are blind until the harness '
                    f'is re-audited ({REAUDIT})') for t in drift]
        tid = finished_tool_use(main_recs)
        if tid and not (STATE / (safe(sid) or 'session') / safe(tid)).exists():
            issues.append(('idle', 'guard idle', 'the guard did not run for the newest finished tool call'))
        accepted = set(x for x in conf.get('accepted_texts') or () if isinstance(x, str))
        hide = {k: v for k, v in {**hide_texts(recs, slot), **(hide_texts(main_recs, None) if main_recs is not recs else {}),
                                  **remember(sid, 'hide')}.items() if k not in accepted}
        if hide:
            issues.append(('hide', f'hide-text {len(hide)}', 'hide-from-user text: '
                           + ', '.join(f'{w} ({h})' for h, w in sorted(hide.items()))))
        issues += [('hooks', 'hooks-off', f'{t} could disable hooks') for t in hooks_off(layers(root))]
        audited = conf.get('audited_version')
        if version and audited and vtuple(version) > vtuple(audited):
            issues.append(('client', cut(f'client {version}', 20), f'client {version} is newer than the audited {audited}'))
        notes += [f'unknown shape: {t}' for t in odd]
        if not gc_path.is_file():
            notes.append(f'no global config at {gc_path.name}')
    except Exception as exc:  # a record of unknown shape: report it, keep the model checks
        log_error(exc)
        issues.append(('blind', 'blind checks', f'a check failed on input of unknown shape ({type(exc).__name__}); the model checks ran'))
    issues.sort(key=lambda i: ORDER.index(i[0]))
    return {'exp': exp[:3], 'issues': issues, 'notes': notes, 'ids': ids, 'hide': hide, 'maps': maps, 'model': model,
            'entry': entry, 'version': version, 'recs': recs, 'reply': reply, 'sid': sid, 'subkeys': subkeys}


def status(d):
    """(experiment label or '', [warning]) for statusline.py; d is Claude Code's status JSON."""
    model = (d.get('model') or {}).get('id') if isinstance(d.get('model'), dict) else None
    r = inspect(d.get('transcript_path'), d.get('session_id'), model, version=d.get('version'), live=True, subagents=True)
    atis = r['exp'][0]
    return (f'exp {atis[:6]}' if atis else ''), [redact(s) for _, s, _ in r['issues']]


def calls(recs, reply, root=ROOT):
    """The tool calls of reply (all its records): tool names, with the repo-relative path of a file tool, never the
    command or other input text (user decision 2026-11-11)."""
    mid, out = (reply.get('message') or {}).get('id'), []
    for e in recs:
        m = e.get('message') if e.get('type') == 'assistant' else None
        if not isinstance(m, dict) or (m is not reply.get('message') and (mid is None or m.get('id') != mid)):
            continue
        for b in m.get('content') if isinstance(m.get('content'), list) else ():
            if isinstance(b, dict) and b.get('type') == 'tool_use':
                inp = b.get('input') if isinstance(b.get('input'), dict) else {}
                f = inp.get('file_path') or inp.get('notebook_path')
                try:
                    f = Path(f).resolve().relative_to(root).as_posix() if isinstance(f, str) else None
                except (ValueError, OSError):
                    f = 'a file outside the repository'
                out.append(f'{cut(b.get("name"))} {f}' if f else cut(b.get('name')))
    return out


def reply_facts(recs):
    """The records model_issues needs, reduced to the newest reply and request: (key, records)."""
    r = newest_reply(recs)
    req = next((e['params'].get('model') for e in reversed(recs) if e.get('type') == 'api-request' and isinstance(e.get('params'), dict)), None)
    out = [{'type': 'api-request', 'params': {'model': req}}] if req is not None else []
    if r:
        out.append({'type': 'assistant', 'requestedModel': requested(recs, r),
                    'message': {'id': obj(r['message']).get('id'), 'model': r['message'].get('model')}})
    return hashlib.sha256(json.dumps(out, sort_keys=True, default=str).encode()).hexdigest()[:16], out


def subagent_state(session, update=None):
    """The session's subagent state {'seen', 'facts', 'refused'}, after merging `update` into the file as it is now,
    so concurrent hooks and status-line runs never drop each other's refusals."""
    f = STATE / (safe(session) or 'session') / 'subagents.json'
    st = load(f)
    st = {'seen': obj(st.get('seen')), 'facts': obj(st.get('facts')), 'refused': [x for x in st.get('refused') or () if isinstance(x, str)]}
    if update:
        st['seen'].update(update.get('seen') or {})
        st['facts'].update(update.get('facts') or {})
        st['refused'] = sorted(set(st['refused']) | set(update.get('refused') or ()))
        f.parent.mkdir(parents=True, exist_ok=True)
        tmp = f.with_name(f'.subagents.{os.getpid()}')
        tmp.write_text(json.dumps(st))
        os.replace(tmp, f)
    return st


def subagent_issues(main, session, ids):
    """([(kind, short, long)], [key]) for the session's subagents: a subagent's last reply reaches its file only after
    its SubagentStop hook ran (user decision 2026-11-11), so the newest reply and request of each transcript under
    <session>/subagents that changed since the last check are kept and checked against the current allowed set until
    a main-thread PreToolUse refuses a call for that reply (refuse)."""
    st, seen, facts = subagent_state(session), {}, {}
    d = main.with_suffix('') / 'subagents'
    for p in sorted(d.glob('agent-*.jsonl')) if d.is_dir() else ():
        try:
            key = [p.stat().st_mtime_ns, p.stat().st_size]
        except OSError:
            continue
        if st['seen'].get(p.name) != key:
            seen[p.name], facts[p.name] = key, reply_facts(tail(p))
    if seen:
        st = subagent_state(session, {'seen': seen, 'facts': facts})
    issues, keys = [], []
    for name, v in sorted(st['facts'].items()):
        k, recs = (v if isinstance(v, (list, tuple)) and len(v) == 2 and isinstance(v[1], list) else (None, []))  # a tuple until stored
        found = [i for i in model_issues([x for x in recs if isinstance(x, dict)], ids) if i[0] == 'model'] if k not in st['refused'] else []
        issues += [(kind, f'sub {sh}', f'subagent {cut(Path(name).stem)}: {t}') for kind, sh, t in found]
        keys += [k] if found else []
    return issues, keys


def refuse(session, keys):
    if keys:
        subagent_state(session, {'refused': keys})


def slipped(r):
    """The undo note for a reply that failed the served-model check after its own tools ran, or ''."""
    if not r['reply'] or not any(k == 'model' and not t.startswith(('the newest request', 'subagent ')) for k, _, t in r['issues']):
        return ''
    done = calls(r['recs'], r['reply'])
    return (' That reply called ' + ', '.join(done) + '. ' + UNDO) if done else ''


def deny(text):
    return {'continue': False, 'stopReason': text, 'hookSpecificOutput': {
        'hookEventName': 'PreToolUse', 'permissionDecision': 'deny', 'permissionDecisionReason': text}}


def warn_once(session, issues):
    """The issues not yet warned about in this session, recording them as warned."""
    f = STATE / (safe(session) or 'session') / 'warned.json'
    old = load(f)
    new = [t for _, s, t in issues if s not in old]
    if new:
        old.update({s: 1 for _, s, _ in issues})
        f.parent.mkdir(parents=True, exist_ok=True)
        tmp = f.with_name(f'.warned.{os.getpid()}')
        tmp.write_text(json.dumps(old))
        os.replace(tmp, f)
    return new


def hook(data):
    """PreToolUse: deny the call and stop the turn when the newest request asks for a model outside the allowed set, or
    the newest reply on disk was served by a model other than its requested one or either lies outside it; the
    calling reply itself reaches the transcript only after its tools ran, so its served model is checked at the next
    call or by the Stop hook. Warn once per session about the other issues. Stop and SubagentStop: warn when the
    newest reply on disk fails the model check; a turn's final text reply is written only after these hooks ran, so
    UserPromptSubmit (user decision 2026-11-11) warns about it when the next prompt is sent, before the model runs.
    SessionStart: show the status-line warnings where no status line runs (entrypoints other than the terminal)."""
    event, session = data.get('hook_event_name'), data.get('session_id')
    path = transcript(data)
    if event == 'SessionStart':
        r = inspect(path, session, live=True)
        if r['issues'] and (r['entry'] or 'cli') != 'cli':
            return {'systemMessage': redact('Harness: ' + '; '.join(t for _, _, t in r['issues']))}
        return None
    r = inspect(path, session, live=True, guard=True, subagents=not data.get('agent_id'))  # a subagent's own hook: its thread only
    bad, session = [t for k, _, t in r['issues'] if k == 'model'], r['sid']
    if event == 'PreToolUse':
        out = deny(redact('Model guard: ' + '; '.join(bad) + '. The tool call was refused and the turn stopped.' + slipped(r) + ' ' + HOW)) if bad else None
        try:  # the heartbeat and state; a failure never cancels the decision
            refuse(session, r['subkeys'] if out else [])  # once per failed subagent reply
            recs, reply, tid = r['recs'], r['reply'], data.get('tool_use_id')
            mine = next((e for e in reversed(recs) if isinstance(tid, str) and e.get('type') == 'assistant' and any(
                isinstance(b, dict) and b.get('id') == tid for b in obj(e.get('message')).get('content') or ())), None)
            checked = obj(reply.get('message')).get('id') if reply else None
            current = bool(mine and checked is not None and obj(mine.get('message')).get('id') == checked)
            newest_req = next((e.get('id') for e in reversed(recs) if e.get('type') == 'api-request'), None)
            h = lambda x: hashlib.sha256(str(x).encode()).hexdigest()[:10]
            info = {'time': time.time(), 'thread': 'subagent' if path and main_of(path) != path else 'main', 'denied': bool(bad),
                    'record': 'current' if current else 'found' if mine else 'absent', 'checked': h(checked), 'request': h(newest_req)}
            beat(session, tid, info)
            remember(session, 'hide', r['hide'])
        except Exception as exc:
            log_error(exc)
        if out:
            return out
        new = warn_once(session, [i for i in r['issues'] if i[0] != 'idle'])
        return {'systemMessage': redact('Harness: ' + '; '.join(new))} if new else None
    if event in ('Stop', 'SubagentStop'):
        try:  # evidence for the limits: is the reply that ended the turn on disk (text reply after the last user record)
            at = next((i for i, e in enumerate(r['recs']) if e is r['reply']), -1)
            asked = max((i for i, e in enumerate(r['recs']) if e.get('type') == 'user'), default=-1)
            f = STATE / (safe(session) or 'session') / 'stop.json'
            old = load(f)
            tools = [cut(b.get('name')) for b in obj((r['reply'] or {}).get('message')).get('content') or () if isinstance(b, dict) and b.get('type') == 'tool_use']
            old[event] = {'time': time.time(), 'reply_after_last_user': at > asked, 'reply_tools': tools}
            f.parent.mkdir(parents=True, exist_ok=True)
            tmp = f.with_name(f'.stop.{os.getpid()}')
            tmp.write_text(json.dumps(old))
            os.replace(tmp, f)
        except Exception as exc:
            log_error(exc)
    if event in ('Stop', 'SubagentStop', 'UserPromptSubmit') and bad:
        at = next((i for i, e in enumerate(r['recs']) if e is r['reply']), -1)
        prompt = max((i for i, e in enumerate(r['recs']) if e.get('type') == 'user' and isinstance(obj(e.get('message')).get('content'), str)), default=-1)
        which = 'a reply of this turn' if event != 'UserPromptSubmit' and at > prompt else "the previous turn's last reply"
        which = which if any(not t.startswith('subagent ') for t in bad) else 'a subagent reply'
        return {'systemMessage': redact(f'Model guard: {which} failed the model check: ' + '; '.join(bad) + '.' + slipped(r) + ' ' + HOW)}
    return None


def log_error(exc):
    try:
        STATE.mkdir(parents=True, exist_ok=True)
        f = STATE / 'errors.log'
        lines = (f.read_text(errors='replace').splitlines() if f.is_file() else [])[-99:]
        lines.append(f'{time.strftime("%Y-%m-%dT%H:%M:%S")} {type(exc).__name__}: {redact(exc)[:300]}')
        f.write_text('\n'.join(lines) + '\n')
    except OSError:
        pass


def selftest(session):
    """(ok, lines): hooks registered in the effective settings, none disabled, a fresh guard heartbeat."""
    ls, lines = layers(), []
    checks = [(registered(ls, 'PreToolUse', every=True), 'a PreToolUse hook for every tool runs harness.py hook')]
    checks += [(registered(ls, e), f'a {e} hook runs harness.py hook') for e in ('Stop', 'SubagentStop', 'UserPromptSubmit', 'SessionStart')]
    checks += [(False, f'{t} could disable hooks') for t in hooks_off(ls)]
    last = STATE / safe(session) / 'last.json' if safe(session) else None
    age = time.time() - last.stat().st_mtime if last and last.is_file() else None
    checks.append((age is not None and age <= FRESH, f'guard heartbeat {f"{age:.0f} s ago" if age is not None else "missing"} '
                   f'(at most {FRESH} s)' if safe(session) else 'guard heartbeat unchecked: CLAUDE_CODE_SESSION_ID is unset'))
    ids, bad = allowed()
    checks.append((bool(ids), f'allowed models: {", ".join(sorted(ids)) or "none"}'))
    lines = [f'{"ok  " if ok else "FAIL"} {redact(t)}' for ok, t in checks] + [f'note {redact(t)}' for t in bad]
    return all(ok for ok, _ in checks), lines


def report(path, session):
    r = inspect(path, session)
    atis, key, source = r['exp']
    out = [f'transcript: {Path(path).name if path else "none found"}; model {r["model"]}; entrypoint {r["entry"]}; client {r["version"]}',
           f'experiment: {atis + " " + (key or "(key not cached)") if atis else "none"} (from {source})',
           'mappings: ' + (', '.join(f'{p}: {a} -> {b}' for p, a, b in r['maps']) or 'none'),
           f'allowed models: {", ".join(sorted(r["ids"])) or "none"}']
    out += [f'{k}: {t}' for k, _, t in r['issues']] or ['issues: none']
    out += [f'note: {t}' for t in r['notes']]
    return '\n'.join(redact(x) for x in out)


def main(argv):
    if argv[:1] == ['hook']:
        try:
            out = hook(json.load(sys.stdin))
        except Exception as exc:  # The guard fails open on its own errors and logs them.
            log_error(exc)
            out = None
        if out:
            print(json.dumps(out))
        return 0
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--selftest', action='store_true', help='exit 1 unless the guard is active')
    ap.add_argument('--experiment', action='store_true', help="print the session's experiment id and key, or none")
    ap.add_argument('--session', help='session id (default: $CLAUDE_CODE_SESSION_ID)')
    ap.add_argument('--transcript', help='transcript path (default: the session\'s)')
    args = ap.parse_args(argv)
    session = args.session or os.environ.get('CLAUDE_CODE_SESSION_ID')
    path = transcript(session=session, path=args.transcript)
    if args.selftest:
        ok, lines = selftest(session)
        print('\n'.join(lines + [f'selftest: {"PASS" if ok else "FAIL: the guard is inactive"}']))
        return 0 if ok else 1
    if args.experiment:
        exp = inspect(path, session)['exp']
        print(redact(f'{exp[0]} {exp[1] or "-"}' if exp[0] else 'unknown' if exp[2] in ('blind', 'unknown') else 'none'))
        return 0
    print(report(path, session))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
