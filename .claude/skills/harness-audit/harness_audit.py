#!/usr/bin/env python3
"""Harness audit scan (SUP29; evidence and limits: 00-Guidelines/13-Plan/Docs/00-harness-audit.md, Weekly scan).

  harness_audit.py scan [--due] [--canary | --no-canary]
                                             fingerprint this clone's Claude Code records, diff it against the
                                             previous scan and write the report; --due: only when the weekly scan or
                                             a canary retry is due (the startup hook); --canary: run the canary now;
                                             --no-canary: never (the skill's plain scan)
  harness_audit.py report                    the newest report's path and its findings not yet reviewed
  harness_audit.py reviewed                  mark the newest report's findings reviewed

Reads the transcript folders of the clone and its worktrees, ~/.claude.json, the cache, settings and session-env folders
of the config folder and the project's .claude, never .credentials.json, and writes only under the git-ignored
.claude/harness-audit/ of the main checkout: the per-file cache (size, mtime, read offset and summary of every
transcript, kept after Claude Code deletes it), the fingerprint history, the texts the findings quote, the report and its
status. One scan runs at a time (scan.lock). The canary (opt-in per clone: `git config compprog.canary true`) runs the
`canary` tasks of .claude/harness.json through headless `claude -p` in a scratch copy of HEAD."""
import argparse
import contextlib
import datetime
import functools
import hashlib
import io
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / '.claude/hooks'))
import harness as H  # noqa: E402

MAIN = H.main_checkout(ROOT)
AUDIT = H.AUDIT
WEEK = H.WEEK
KEEP = 100  # fingerprints kept in history/
TEXT_CAP = 4000  # characters stored per quoted text
EARLY = 0.9  # an automatic compaction below this share of autoCompactWindow is early; a tool's cap below this share
             # of its largest earlier inline result is lowered
SHARE = 0.99  # the lowest 1-hour share of cache writes that is not a finding
DYNAMIC = 20  # an attachment type with this many distinct texts holds session data: listed, never a finding
HEXRUN = re.compile(r'\b(?=[0-9a-f]*\d)[0-9a-f]{8,}\b', re.I)
RECORD = ('record_types', 'attachment_types', 'system_subtypes', 'record_keys', 'message_keys', 'model_attachment_keys', 'usage_keys',
          'input_transformations', 'cache_miss_reasons', 'stop_reasons', 'betas', 'request_params', 'request_values',
          'query_sources', 'served_models', 'requested_models', 'efforts', 'latches', 'versions', 'shapes', 'prompts',
          'injected', 'directives', 'parse_errors')
SNAPSHOT = ('global_keys', 'growthbook', 'gb_texts', 'hide', 'tally', 'experiments', 'slot_fields', 'slot_keys', 'slot_texts',
            'files', 'env_names', 'managed', 'catalog', 'aliases', 'memory')
OWN = {'prompts', 'injected', 'directives', 'memory', 'shapes', 'versions', 'tally', 'managed', 'gb_texts', 'slot_texts', 'hide'}  # findings of their own
NEEDED = {  # what a SUP28 check reads (harness.py); its removal is listed first
    'record_types': {'assistant', 'user', 'attachment', 'system', 'api-request', 'atis-latch'},
    # keys every record of the type carries (user decision 2026-11-11): an optional one, such as attributionSkill on
    # skill turns or rendered, would read as removed after a week without it; its removal is still a diff finding
    'record_keys': {'assistant.message', 'assistant.requestedModel', 'assistant.requestRef', 'assistant.effort',
                    'assistant.perTurnEffort', 'assistant.sessionId', 'assistant.entrypoint', 'assistant.version',
                    'api-request.params', 'api-request.id', 'api-request.querySource', 'atis-latch.atis',
                    'attachment.attachment', 'system.subtype', 'user.message'},
    'model_attachment_keys': {'identity.modelId'},
    'message_keys': {'model', 'id', 'content'},
    'request_params': {'model', 'max_tokens', 'thinking', 'context_management', 'output_config.effort'},
    'attachment_types': {'model'},
    'global_keys': {'clientDataCacheSlots'},
    'slot_fields': {'at', 'model', 'entrypoint', 'data'},
}
ITEMS = {'backups', 'debug', 'downloads', 'file-history', 'ide', 'jobs', 'paste-cache', 'session-env', 'sessions',
         'shell-snapshots', 'tasks', 'todos'}  # config folders of per-session or per-item files with random names
FILE_KEYS = H.SOURCE_KEYS - {'prompt', 'hookName', 'hookEvent'}  # an attachment holding a file
OVERRIDE = {'model', 'effortLevel', 'availableModels', 'modelSettings', 'alwaysThinkingEnabled', 'hooks', 'agent'}
PROMPT = ('The header `{remove}` is missing from this library. Write it so that it provides every operation of its row in '
          "the folder's `00-index.md`, with the contracts of the row's evidence document under the folder's `Docs/` and "
          "of `Docs/00-notes.md`, under the rules of `00-Guidelines/03-cpp.md`. Edit no other file. "
          '`python3 {test}` checks it.')


def digest(v, n=12):
    return hashlib.sha256((v if isinstance(v, str) else json.dumps(v, sort_keys=True, default=str)).encode()).hexdigest()[:n]


def norm(s):
    """A text with secrets, hex runs and digits normalized, so a date or a session id makes no new text."""
    return re.sub(r'\d+', '0', HEXRUN.sub('<hex>', H.SECRET.sub('<redacted>', s))).strip()


def keep(texts, s):
    """The hash of s normalized; s is stored under it, secrets redacted, the first time it is seen."""
    h = digest(norm(s))
    texts.setdefault(h, H.SECRET.sub('<redacted>', s).strip()[:TEXT_CAP])
    return h


@functools.lru_cache(maxsize=1 << 16)
def clean(item):
    """A fingerprint item redacted (items repeat across records, hence the cache), so no cache, history or report
    file holds a secret a record carried."""
    return H.redact(item)


def hide_matches(s):
    """[(hash, sentence)] for the hide-from-user matches in s: the hash is harness.py's, of the raw sentence; the
    sentence shown is cut at the same match from s with each secret masked at its own length, then the masks redacted,
    since a sentence can end inside an e-mail address."""
    ms = [m for _, m in zip(range(20), H.HIDE.finditer(s))]
    if not ms:
        return []
    masked = H.SECRET.sub(lambda x: '\x01' * len(x.group()), s)
    return [(digest(H.sentence(s, m)), re.sub('\x01+', '<redacted>', H.sentence(masked, m))[:300]) for m in ms]


def scanned(texts):
    """The tags of harness.py's instruction-like blocks and the `prose:<kind>` of its unwrapped directives in texts,
    one per occurrence, within harness.py's per-response budget."""
    out, left = [], H.SCAN_CAP
    for s in texts:
        if left <= 0:
            break
        if isinstance(s, str):
            blocks, lines = H.scan(s[:left])
            left -= len(s)
            out += [b[2] for b in blocks] + [f'prose:{x[2]}' for x in lines]
    return out


def text_of(v):
    return v if isinstance(v, str) else ''


def epoch(v):
    """Seconds since 1970 of an epoch (s or ms) or ISO time between 1970 and 2286, else None."""
    if isinstance(v, (int, float)) and not isinstance(v, bool):
        t = v / 1000 if v > 1e11 else v
    else:
        try:
            t = datetime.datetime.fromisoformat(str(v).replace('Z', '+00:00')).timestamp()
        except (TypeError, ValueError, OverflowError):
            return None
    return t if 0 < t < 1e10 else None  # also drops NaN and infinities, which every comparison rejects


def day(t):
    try:
        return time.strftime('%Y-%m-%d', time.gmtime(t)) if t else '?'
    except (TypeError, ValueError, OverflowError, OSError):
        return '?'


def leaves(v, path='', depth=0):
    """(dotted path, scalar) for every leaf, lists as [], to depth 12 (harness.py's strings)."""
    if isinstance(v, dict) and depth < 12:
        for k, x in v.items():
            yield from leaves(x, f'{path}.{k}' if path else str(k), depth + 1)
    elif isinstance(v, list) and depth < 12:
        for x in v:
            yield from leaves(x, path + '[]', depth + 1)
    else:
        yield path, v


def paths(v, path='', depth=0):
    """Every dotted key path of v, inner nodes included."""
    if isinstance(v, dict) and depth < 6:
        for k, x in v.items():
            p = f'{path}.{k}' if path else str(k)
            yield p
            yield from paths(x, p, depth + 1)
    elif isinstance(v, list) and depth < 6:
        for x in v[:8]:
            yield from paths(x, path + '[]', depth + 1)


def save(path, obj):
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(f'.{path.name}.{os.getpid()}')
    tmp.write_text(json.dumps(obj, sort_keys=True))
    os.replace(tmp, path)


def empty():
    return {'record': {}, 'metrics': {'exp': {}, 'cache': {}, 'tools': {}, 'compactions': {}, 'hide': {}}}


def extreme(f, a, b):
    return b if a is None else a if b is None else f(a, b)


def merge(into, s):
    """Add summary s into summary into: counts add, first and last widen, metric sums add, caps keep the extremes."""
    for sec, items in s['record'].items():
        d = into['record'].setdefault(sec, {})
        for k, (n, a, b) in items.items():
            o = d.get(k)
            d[k] = [n + o[0], min(a, o[1]), max(b, o[2])] if o else [n, a, b]
    m, sm = into['metrics'], s['metrics']
    for sec in ('exp', 'cache'):
        for k, v in sm[sec].items():
            o = m[sec].get(k)
            m[sec][k] = [x + y for x, y in zip(o, v)] if o else list(v)
    for k, (hi, lo, p, c) in sm['tools'].items():
        o = m['tools'].get(k, [None, None, 0, 0])
        m['tools'][k] = [extreme(max, o[0], hi), extreme(min, o[1], lo), o[2] + p, o[3] + c]
    for sec in ('compactions', 'hide'):
        for k, v in sm[sec].items():
            m[sec].setdefault(k, v)
    return into


class Reader:
    """Absorbs the records of one transcript into a summary; `st` is the file's parse state (newest time, latch,
    recent tool ids and message ids), kept in the cache so an appended file is read from its last offset only."""

    def __init__(self, st, sub, latch, texts):  # latch: the session's, for a subagent transcript (it has none)
        self.st, self.sub, self.latch, self.texts, self.s = st, sub, latch, texts, empty()
        self.now = time.time() + 300  # a record time past the scan (5 minutes for clock skew) is not one

    def see(self, sec, item, ts):
        d, item = self.s['record'].setdefault(sec, {}), clean(item)
        o = d.get(item)
        d[item] = [o[0] + 1, min(o[1], ts), max(o[2], ts)] if o else [1, ts, ts]

    def text(self, t):
        return keep(self.texts, t)

    def absorb(self, e):
        st = self.st
        t = epoch(e.get('timestamp'))  # a record without a time, or one after the scan, takes the previous record's
        st['ts'] = ts = t if t and t <= self.now else st['ts']
        t = e.get('type')
        t = t if isinstance(t, str) else f'<{type(t).__name__}>'
        self.see('record_types', t, ts)
        for k in e:
            self.see('record_keys', f'{t}.{k}', ts)
        if isinstance(e.get('version'), str):
            self.see('versions', e['version'], ts)
        v = e.get('version') if isinstance(e.get('version'), str) else '?'
        if t in ('attachment', 'system'):
            for h, where in H.hide_texts([e], None).items():
                self.s['metrics']['hide'].setdefault(h, [where, self.hide_sentence(e, h)])
        if t == 'attachment':
            a = H.obj(e.get('attachment'))
            self.see('attachment_types', str(a.get('type')), ts)
            for tool, tag in H.integrity_items(a):  # the PostToolUse context-integrity notes (SUP30)
                self.see('directives', f'note {tool} {tag}', ts)
            if any(k in FILE_KEYS for d in H.parts(a) for k in d):  # an @-mentioned or loaded file, as the model read it
                for tag in scanned(H.strings(a)):
                    self.see('directives', f'file {a.get("type")} {tag}', ts)
            if a.get('type') == 'model':  # harness.py reads its identity.modelId
                for p in paths(a):
                    self.see('model_attachment_keys', p, ts)
            if a and not any(k in H.SOURCE_KEYS for d in H.parts(a) for k in d):
                rendered = [x.get('content') for x in e.get('rendered') if isinstance(x, dict) and isinstance(x.get('content'), str)] \
                    if isinstance(e.get('rendered'), list) else []
                body = '\n'.join(rendered) or '\n'.join(s for k, x in a.items() if k != 'type' for s in H.strings(x))
                if body.strip():
                    self.see('injected', f'{a.get("type")} {self.text(body)}', ts)
        elif t == 'system':
            self.see('system_subtypes', str(e.get('subtype')), ts)
            m, comp = H.obj(e.get('compactMetadata')), self.s['metrics']['compactions']
            if e.get('subtype') == 'compact_boundary':
                st['boundary'] = digest(str(e.get('uuid') or digest(e)))  # ids are stored as digests
                idle = abs(ts - st.get('idle', -1e18)) < 300
                comp[st['boundary']] = [ts, f'{m.get("trigger")}{" (idle)" if idle else ""}', m.get('preTokens'), v]
            elif 'while idle' in str(e.get('content')):  # the client's idle-compaction notice, before or after its boundary
                st['idle'] = ts
                c = comp.get(st.get('boundary'))
                if c and abs(ts - c[0]) < 300 and not c[1].endswith('(idle)'):
                    c[1] += ' (idle)'
        elif t == 'atis-latch':
            a = e.get('atis')
            self.see('latches', a or '(empty)' if isinstance(a, str) else f'<{type(a).__name__}>', ts)
            st['latch'] = a or None if isinstance(a, str) else st.get('latch')
        elif t == 'api-request':
            p = H.obj(e.get('params'))
            for path in paths(p):
                if not path.startswith('betas'):
                    self.see('request_params', path, ts)
            for path, x in leaves(p):
                if not path.startswith('betas') and x is not None:
                    self.see('request_values', f'{path}={H.cut(x, 60)}', ts)
            for b in p.get('betas') or () if isinstance(p.get('betas'), list) else ():
                self.see('betas', str(b), ts)
            self.see('requested_models', str(p.get('model')), ts)
            self.see('efforts', f'request {H.obj(p.get("output_config")).get("effort")}', ts)
            self.see('query_sources', str(e.get('querySource')), ts)
            if isinstance(e.get('shapeHash'), str):
                self.see('shapes', f'{v} {e["shapeHash"][:16]}', ts)
        elif t == 'api-request-shape':
            self.shape(e, v, ts)
        elif t == 'assistant':
            self.reply(e, ts)
        elif t == 'user':
            self.results(e)

    def hide_sentence(self, e, h):
        for s in H.strings([H.obj(e.get('attachment')), e.get('rendered'), e.get('content')]):
            for hh, sentence in hide_matches(s):
                if hh == h:
                    return sentence
        return ''

    def shape(self, e, v, ts):
        h = str(e.get('shapeHash'))[:16]
        self.see('shapes', f'{v} {h}', ts)
        if h in self.st['shapes']:
            return
        self.st['shapes'] = (self.st['shapes'] + [h])[-50:]
        sh = H.obj(e.get('shape'))
        for b in sh.get('system') or () if isinstance(sh.get('system'), list) else ():
            for para in re.split(r'\n\s*\n', text_of(H.obj(b).get('text'))):
                if para.strip():
                    self.see('prompts', f'{v} system {self.text(para)}', ts)
        for tl in sh.get('tools') or () if isinstance(sh.get('tools'), list) else ():
            name = str(H.obj(tl).get('name'))
            for para in re.split(r'\n\s*\n', text_of(H.obj(tl).get('description'))):
                if para.strip():
                    self.see('prompts', f'{v} tool {name} {self.text(para)}', ts)
            self.see('prompts', f'{v} schema {name} {digest(H.obj(tl).get("input_schema"))}', ts)

    def reply(self, e, ts):
        m, st = H.obj(e.get('message')), self.st
        for k, x in m.items():
            self.see('message_keys', k, ts)
            for k2 in H.obj(x) if k in ('diagnostics', 'stop_details', 'context_management') else ():
                self.see('message_keys', f'{k}.{k2}', ts)
        u = H.obj(m.get('usage'))
        for path in paths(u):
            self.see('usage_keys', path, ts)
        for x in m.get('input_transformations') or () if isinstance(m.get('input_transformations'), list) else ():
            self.see('input_transformations', f'{H.obj(x).get("type")} {H.obj(x).get("reason")}', ts)
        miss = H.obj(H.obj(m.get('diagnostics')).get('cache_miss_reason')).get('type')
        if miss is not None:
            self.see('cache_miss_reasons', str(miss), ts)
        self.see('stop_reasons', str(m.get('stop_reason')), ts)
        self.see('served_models', str(m.get('model')), ts)
        self.see('efforts', f'reply {e.get("effort")}', ts)
        self.see('efforts', f'per-turn {e.get("perTurnEffort")}', ts)
        for b in m.get('content') or () if isinstance(m.get('content'), list) else ():
            if isinstance(b, dict) and b.get('type') == 'tool_use' and isinstance(b.get('id'), str):
                st['tools'][digest(b['id'])] = clean(str(b.get('name')))  # ids are stored as digests
        if len(st['tools']) > 200:
            st['tools'] = dict(list(st['tools'].items())[-200:])
        if m.get('model') == '<synthetic>' or not isinstance(m.get('id'), str) or not u:
            return
        mid = digest(m['id'])
        out = u.get('output_tokens') if isinstance(u.get('output_tokens'), int) else 0
        think = H.obj(u.get('output_tokens_details')).get('thinking_tokens')
        think = think if isinstance(think, int) else 0
        key = clean(f'{(st.get("latch") if not self.sub else self.latch) or "none"}|{m.get("model")}|{e.get("effort")}|{"sub" if self.sub else "main"}')
        old = st['mids'].get(mid)
        ex = self.s['metrics']['exp']
        if old is None:
            cw = H.obj(u.get('cache_creation'))
            h1, m5 = (cw.get(k) if isinstance(cw.get(k), int) else 0 for k in ('ephemeral_1h_input_tokens', 'ephemeral_5m_input_tokens'))
            c = self.s['metrics']['cache'].setdefault(f'{day(ts)} {"sub" if self.sub else "main"}', [0, 0])
            c[0], c[1] = c[0] + h1, c[1] + m5
            ex[key] = [x + y for x, y in zip(ex.get(key, [0, 0, 0]), (1, out, think))]
            st['mids'][mid] = [out, think, key]
        elif out > old[0] or think > old[1]:  # a later record of the reply carries the final usage
            k0 = old[2]
            add = (0, max(out - old[0], 0), max(think - old[1], 0))
            ex[k0] = [x + y for x, y in zip(ex.get(k0, [0, 0, 0]), add)]
            st['mids'][mid] = [max(out, old[0]), max(think, old[1]), k0]
        if len(st['mids']) > 64:
            st['mids'] = dict(list(st['mids'].items())[-64:])

    def results(self, e):
        c = H.obj(e.get('message')).get('content')
        for b in c if isinstance(c, list) else ():
            if not (isinstance(b, dict) and b.get('type') == 'tool_result'):
                continue
            body = b.get('content')
            body = text_of(body) if not isinstance(body, list) else '\n'.join(text_of(H.obj(x).get('text')) for x in body)
            tool = self.st['tools'].get(digest(str(b.get('tool_use_id'))), '?')
            if tool not in H.TRUSTED and tool not in H.FILE_TOOLS:  # untrusted output as the model saw it (SUP30)
                for tag in scanned([H.CLIENT.sub(H.blank, body)]):  # the client's own wrapper is no sighting
                    self.see('directives', f'result {tool} {tag}', self.st['ts'])
            hi, lo, p, cut = self.s['metrics']['tools'].get(tool, [None, None, 0, 0])
            big = re.search(r'Output too large \(([\d.]+)\s*(K|M)?B\)', body) if '<persisted-output>' in body else None
            if big:
                lo, p = extreme(min, lo, float(big.group(1)) * {'K': 1024, 'M': 1 << 20}.get(big.group(2), 1)), p + 1
            elif re.search(r'\[\d+ characters truncated\]', body):
                cut += 1
            else:
                hi = extreme(max, hi, len(body.encode(errors='replace')))
            self.s['metrics']['tools'][tool] = [hi, lo, p, cut]


def project_folders():
    """The transcript folders of this clone: its main checkout exactly, its top-level folders and their subfolders,
    and its worktrees (listed by git or under .claude/worktrees, deleted ones included)."""
    proj = H.config_dir() / 'projects'
    enc = lambda p: re.sub(r'[^A-Za-z0-9]', '-', str(p))
    top = enc(MAIN)
    stems = [enc(d) for d in MAIN.iterdir() if d.is_dir() and d.name != '.git'] if MAIN.is_dir() else []
    for g in (MAIN / '.git/worktrees').glob('*/gitdir') if (MAIN / '.git/worktrees').is_dir() else ():
        with contextlib.suppress(OSError):
            stems.append(enc(Path(g.read_text().strip()).parent))
    try:
        folders = [f for f in proj.iterdir() if f.is_dir()]
    except OSError:
        return []
    return sorted(f for f in folders if f.name == top or any(f.name == s or f.name.startswith(s + '-') for s in stems))


def transcripts():
    """[(key, path, main key or None)]: every main transcript, then every subagent transcript with its session's; the
    keys are digests of the relative paths, which hold session ids."""
    folders, k = project_folders(), lambda s: digest(s, 16)
    out = [(k(f'{f.name}/{p.name}'), p, None) for f in folders for p in sorted(f.glob('*.jsonl'))]
    return out + [(k(f'{f.name}/{p.relative_to(f).as_posix()}'), p, k(f'{f.name}/{p.parent.parent.name}.jsonl'))
                  for f in folders for p in sorted(f.glob('*/subagents/*.jsonl'))]


def bhash(b):
    return hashlib.sha256(b).hexdigest()[:16]


def read_file(path, entry, sub, latch, texts):
    """Read the bytes of path added since entry (its cache record) was written, from its offset when the file only
    grew, else from the start; returns the summary of those bytes, or None when size and mtime are unchanged."""
    st = path.stat()
    if entry.get('size') == st.st_size and entry.get('mtime') == st.st_mtime_ns:
        return None
    with open(path, 'rb') as f:
        head = f.read(4096)
        off = entry.get('offset', 0)
        if not off or st.st_size < off or entry.get('head') != bhash(head[:off]):  # new, shrunk or rewritten
            entry.clear()
            entry.update(sum=empty(), state={'ts': min(st.st_mtime_ns / 1e9, time.time()), 'latch': None, 'tools': {}, 'mids': {}, 'shapes': []})
            off = 0
        f.seek(off)
        r, end = Reader(entry['state'], sub, latch, texts), off
        for line in f:  # line by line: a transcript can hold hundreds of megabytes
            if not line.endswith(b'\n'):
                break  # a line still being written is read next time
            end += len(line)
            try:
                e = json.loads(line)
            except (ValueError, RecursionError):
                continue
            if isinstance(e, dict):
                try:
                    r.absorb(e)
                except Exception as exc:  # a record of unknown shape is a finding, never the end of the scan
                    r.see('parse_errors', f'{H.cut(e.get("type"))} {type(exc).__name__}', r.st['ts'])
    entry.update(size=st.st_size, mtime=st.st_mtime_ns, offset=end, head=bhash(head[:end]))
    merge(entry['sum'], r.s)
    return r.s


def snapshot(texts):
    """The current state of the global config, the config folder and the project's settings, as {section: {item: value}}."""
    snap = {sec: {} for sec in SNAPSHOT}
    gp = H.global_config()
    gc = H.load(gp)

    def text(sec, item, s):
        snap[sec][item] = keep(texts, s)
    for k, v in gc.items():
        snap['global_keys'][k] = type(v).__name__
    def strings(sec, v, path):  # every string is scanned for hide-text, the prompt-like ones are kept
        for p, s in leaves(v, path):
            if isinstance(s, str):
                for h, sentence in hide_matches(s):
                    snap['hide'].setdefault(h, [f'{sec} {p}', sentence])
                if len(s) >= 40 and ' ' in s:
                    text(sec, p, s)
    gb = H.obj(gc.get('cachedGrowthBookFeatures'))
    for k, v in gb.items():
        snap['growthbook'][k] = digest(v)
        strings('gb_texts', v, k)
    for p, x in leaves(gb.get('tengu_amber_tally')):
        snap['tally'][p or '(value)'] = json.dumps(x)
    for k, v in H.obj(gc.get('cachedExperimentData')).items():
        snap['experiments'][k] = f'{H.obj(v).get("experimentId")} v{H.obj(v).get("variationId")}'
    for s in H.slots(gc):
        for k in s:
            snap['slot_fields'][k] = '1'
        d = s['data']
        snap['experiments'][f'slot {s.get("model")} {s.get("entrypoint")}'] = f'{d.get("atis") or "-"} {d.get("experimentKey") or "-"}'
        for k, v in d.items():
            snap['slot_keys'][k] = type(v).__name__
            strings('slot_texts', v, k)
    for folder in project_folders():  # the clone's auto-memory, which Claude writes and reads back (SUP30)
        for f in sorted((folder / 'memory').glob('*.md')):
            with contextlib.suppress(OSError):
                for tag in scanned([f.read_text(errors='replace')]):
                    item = f'{f.name} {tag}'
                    snap['memory'][item] = snap['memory'].get(item, 0) + 1
    cfg = H.config_dir()
    snap['files'] = {**names(cfg, 'config'), **names(MAIN / '.claude', 'project')}
    ls = H.layers(MAIN)
    for label, s in ls:
        for k in H.obj(s.get('env')):
            snap['env_names'][f'{label}: {k}'] = '1'
        if label.startswith('managed'):
            for k, v in s.items():
                if k in OVERRIDE or 'hook' in k.lower():
                    snap['managed'][f'{label}: {k}'] = digest(v)
            for k, v in H.obj(s.get('env')).items():
                if re.search(r'MODEL|EFFORT|THINK|OUTPUT_TOKENS|HOOK', k):
                    snap['managed'][f'{label}: env.{k}'] = digest(v)
    for t in H.hooks_off(ls):
        snap['managed'][t] = '1'
    for f in sorted((cfg / 'session-env').rglob('*')) if (cfg / 'session-env').is_dir() else ():
        with contextlib.suppress(OSError, UnicodeDecodeError):
            if f.is_file() and f.stat().st_size < 1 << 20:
                for name in re.findall(r'^\s*(?:export\s+)?([A-Za-z_]\w*)=', f.read_text(), re.M):
                    snap['env_names'][f'session-env: {name}'] = '1'
    for f in sorted((cfg / 'cache/model-catalog').glob('*.json')):
        c = H.obj(H.obj(H.load(f).get('catalog')).get('config'))
        for m in c.get('models') or () if isinstance(c.get('models'), list) else ():
            m, rt = H.obj(m), H.obj(H.obj(m).get('runtime'))
            snap['catalog'][str(m.get('id'))] = f'{m.get("section")}; default effort {rt.get("default_effort")}; max output {rt.get("max_output_tokens")}'
        for alias, t in H.obj(c.get('provider_alias_targets')).items():
            snap['aliases'][alias] = str(H.obj(t).get('default'))
            for prov, target in H.obj(H.obj(t).get('per_provider')).items():
                snap['aliases'][f'{alias}/{prov}'] = str(target)
    # keys and values redacted: a key or value of the client's files can hold an id (a slot key with a UUID)
    return {sec: {clean(k): clean(v) if isinstance(v, str) else v for k, v in items.items()} for sec, items in snap.items()}


def names(base, label):
    """{normalized relative path: '1'} for the names two levels deep (four under projects/), ids normalized; the
    entries of a folder of per-item files (ITEMS) reduce to their extensions, since their names hold random ids."""
    out = {}

    def walk(d, rel, depth, limit):
        try:
            entries = list(os.scandir(d))
        except OSError:
            return
        for x in entries:
            if label == 'project' and not rel and x.name == AUDIT.name:
                continue  # the scan's own output
            n = ('<project>' if rel == 'projects' else '<item>' + ''.join(Path(x.name).suffixes[-1:]) if label == 'config' and rel in ITEMS
                 else norm(x.name))
            r = f'{rel}/{n}' if rel else n
            is_dir = x.is_dir(follow_symlinks=False)
            out[f'{label}:{r}' + ('/' if is_dir else '')] = '1'
            if is_dir and depth < (4 if r.startswith('projects') else limit):
                walk(x.path, r, depth + 1, limit)
    walk(base, '', 1, 2)
    return out


def fingerprint(cache, snap, now):
    total = empty()
    for e in cache['files'].values():
        merge(total, e['sum'])
    newest = max((v[2] for v in total['record'].get('record_types', {}).values()), default=0)
    return {'time': now, 'newest': newest, 'record': total['record'], 'snapshot': snap, 'metrics': total['metrics']}


def diff(prev, cur):
    """{section: (added, removed, changed)}: a record item is removed when it was seen in the week before the
    previous scan's newest record and not in the week before the newest record now."""
    out = {}
    for sec in RECORD:
        a, b = prev['record'].get(sec, {}), cur['record'].get(sec, {})
        added = sorted(set(b) - set(a))
        removed = sorted(k for k, v in a.items() if v[2] >= prev['newest'] - WEEK and b.get(k, v)[2] < cur['newest'] - WEEK)
        if added or removed:
            out[sec] = (added, removed, [])
    for sec in SNAPSHOT:
        a, b = prev['snapshot'].get(sec, {}), cur['snapshot'].get(sec, {})
        added, removed = sorted(set(b) - set(a)), sorted(set(a) - set(b))
        changed = sorted(k for k in set(a) & set(b) if a[k] != b[k])
        if added or removed or changed:
            out[sec] = (added, removed, changed)
    return out


def version_of(item):
    return item.split(' ', 1)[0]


def changelog(old, new):
    """The changelog entries of the versions after old up to new, as lines."""
    try:
        text = (H.config_dir() / 'cache/changelog.md').read_text(errors='replace')
    except OSError:
        return ['(no cached changelog)']
    out, keep = [], False
    for line in text.splitlines():
        m = re.match(r'##\s+(\S+)', line)
        if m:
            keep = H.vtuple(old) < H.vtuple(m.group(1)) <= H.vtuple(new)
        if keep and line.strip():
            out.append(line.strip())
    return out[:80] or [f'(no changelog entry after {old})']


def findings(prev, cur, delta, texts, conf, ids):
    """[{id, kind, title, lines}], removed fields a SUP28 check needs first. A finding that recurs at every scan
    (hide, cache, toolcap, managed, alias) takes its id from a key of its subject, so once reviewed it stays reviewed."""
    out = []

    def add(kind, title, lines=(), key=None):
        lines = [H.redact(x)[:400] for x in lines]
        out.append({'id': digest(kind + (title + '\n'.join(lines) if key is None else key)), 'kind': kind,
                    'title': H.redact(title), 'lines': lines})
    d = diff(prev, cur) if prev else {}
    needed = sorted(f'{sec}: {k}' for sec, (_, removed, _) in d.items() for k in removed if k in NEEDED.get(sec, ()))
    if needed:
        add('needed', f'{len(needed)} removed field(s) a SUP28 check needs: re-audit package P287', needed)
    for sec, (added, removed, changed) in d.items():
        if sec not in OWN:
            add('diff', f'{sec}: {len(added)} new, {len(removed)} removed, {len(changed)} changed',
                [f'+ {k}' for k in added[:30]] + [f'- {k}' for k in removed[:30]] + [f'~ {k}' for k in changed[:30]])
    if 'tally' in d:
        a, b = prev['snapshot']['tally'], cur['snapshot']['tally']
        add('tally', 'the quota-rate table (tengu_amber_tally) changed',
            [f'{k}: {a.get(k, "(none)")} -> {b.get(k, "(none)")}' for k in sorted(set(a) | set(b)) if a.get(k) != b.get(k)][:40])
    for sec in ('gb_texts', 'slot_texts'):
        if sec in d:
            added, removed, changed = d[sec]
            add('texts', f'{sec}: {len(added)} new, {len(changed)} changed prompt texts',
                [f'{k}: {texts.get(cur["snapshot"][sec][k], "")[:300]}' for k in added + changed][:20])
    if prev:
        known = {version_of(k) for k in prev['record'].get('prompts', {})}
        new = sorted(set(cur['record'].get('prompts', {})) - set(prev['record'].get('prompts', {})))
        for v in sorted({version_of(k) for k in new}, key=H.vtuple):
            mine = [k for k in new if version_of(k) == v]
            if v in known:
                title = f'prompt texts changed within client {v}: {len(mine)} new paragraph(s) or schema(s)'
                lines = [f'+ {k.split(" ", 1)[1]}: {texts.get(k.rsplit(" ", 1)[1], "")[:300]}' for k in mine[:12]]
            else:
                older = [x for x in {version_of(k) for k in cur['record'].get('prompts', {})} if H.vtuple(x) < H.vtuple(v)]
                base = max(older, key=H.vtuple) if older else None
                strip = lambda keys, ver: {k.split(' ', 1)[1] for k in keys if version_of(k) == ver}
                now, then = strip(cur['record']['prompts'], v), strip(cur['record']['prompts'], base)
                title = f'client {v}: {len(now - then)} prompt paragraph(s) or schema(s) new and {len(then - now)} gone against {base}'
                lines = [f'+ {k}: {texts.get(k.rsplit(" ", 1)[1], "")[:300]}' for k in sorted(now - then)[:12]]
                lines += [f'- {k}: {texts.get(k.rsplit(" ", 1)[1], "")[:300]}' for k in sorted(then - now)[:12]]
            add('prompts', title, lines)
        counts = {}
        for k in cur['record'].get('injected', {}):
            counts[k.rsplit(' ', 1)[0]] = counts.get(k.rsplit(' ', 1)[0], 0) + 1
        new = [k.rsplit(' ', 1) for k in sorted(set(cur['record'].get('injected', {})) - set(prev['record'].get('injected', {})))]
        for typ in sorted({typ for typ, _ in new}):
            if counts[typ] < DYNAMIC:
                add('injected', f'new injected text of attachment type {typ}', [texts.get(h, '')[:400] for t, h in new if t == typ][:5])
        old_v = max(prev['record'].get('versions', {}) or ['0'], key=H.vtuple)
        new_v = max(cur['record'].get('versions', {}) or ['0'], key=H.vtuple)
        if H.vtuple(new_v) > H.vtuple(old_v):
            add('client', f'client {old_v} -> {new_v}: changelog entries since the last scan', changelog(old_v, new_v))
    got = delta['record'].get('directives', {})
    mem = sorted(set(cur['snapshot'].get('memory', {})) - set(prev['snapshot'].get('memory', {}) if prev else ()))
    if got or mem:  # one finding per scan that read new sightings: hook notes, unescaped results, files, memory
        add('integrity', f'instruction-like content since the last scan: {sum(v[0] for v in got.values())} sighting(s) in '
            f'{len(got)} source, tool and tag group(s), {len(mem)} new in memory files',
            [f'{k}: {v[0]}' for k, v in sorted(got.items())][:30] + [f'memory {k}' for k in mem][:10])
    accepted = {x for x in conf.get('accepted_texts') or () if isinstance(x, str)}
    for h, (where, sentence) in sorted({**cur['snapshot']['hide'], **cur['metrics']['hide']}.items()):
        if h not in accepted:
            add('hide', f'hide-from-user text {h} in {where}', [sentence] if sentence else [], h)
    window = next((H.load(f).get('autoCompactWindow') for f in (MAIN / '.claude/settings.local.json', MAIN / '.claude/settings.json',
                                                                  H.config_dir() / 'settings.json') if isinstance(H.load(f).get('autoCompactWindow'), int)), None)
    early = sorted(v for v in delta['metrics']['compactions'].values()
                   if window and v[1] != 'manual' and isinstance(v[2], int) and v[2] < EARLY * window)
    if early:
        add('compaction', f'{len(early)} compaction(s) before {EARLY:.0%} of autoCompactWindow {window}',
            [f'{day(t)} {trig} at {pre} tokens, client {v}' for t, trig, pre, v in early[-20:]])
    cache = delta['metrics']['cache']
    share = {th: [sum(v[i] for k, v in cache.items() if k.endswith(th)) for i in (0, 1)] for th in (' main', ' sub')}
    low = {th.strip(): round(v[0] / sum(v) * 20) for th, v in share.items() if sum(v) and v[0] / sum(v) < SHARE}
    if low:  # keyed by thread and share in 5% steps
        add('cache', '1-hour share of cache writes since the last scan: ' + ', '.join(
            f'{th.strip()} thread {v[0] / sum(v):.1%} of {sum(v)} tokens' for th, v in share.items() if sum(v)),
            [f'{k}: {v[0] / sum(v):.1%} of {sum(v)}' for k, v in sorted(cache.items()) if sum(v) and v[0] / sum(v) < SHARE][-14:],
            ' '.join(f'{k}:{v}' for k, v in sorted(low.items())))
    for tool, (hi, lo, p, c) in sorted(delta['metrics']['tools'].items()):
        before = prev['metrics']['tools'].get(tool) if prev else None
        # 10% margins, since sizes are rounded; flagged once per new low, keyed by tool and size in KB
        if (lo is not None and before and before[0] is not None and lo < EARLY * before[0]
                and (before[1] is None or lo < EARLY * before[1])):
            add('toolcap', f'{tool} output cap lowered: a {lo / 1024:.1f} KB result was saved to a file; '
                f'results up to {before[0] / 1024:.1f} KB stayed inline before', key=f'{tool} {round(lo / 1024)}')
    for item, v in sorted(cur['snapshot']['managed'].items()):
        add('managed', f'{item} could disable hooks or override the model or effort', key=f'{item} {v}')
    for alias, target in sorted(cur['snapshot']['aliases'].items()):
        fam = [i for i in ids if H.base(i).startswith(f'claude-{alias}-')]
        if '/' not in alias and fam and target not in ids:
            add('alias', f'alias {alias} resolves to {target}, outside the allowed set ({", ".join(sorted(fam))})', key=f'{alias} {target}')
    return out


def metrics_table(fp):
    """The per-experiment output and thinking table (SUP28): replies counted once per message id."""
    rows = fp['metrics']['exp']
    lines = ['| experiment | model | effort | thread | replies | output/reply | thinking/reply | vs none |', '|---|---|---|---|---|---|---|---|']
    for k in sorted(rows):
        n, out, think = rows[k]
        exp, model, effort, thread = (k.split('|') + ['?'] * 4)[:4]
        none = rows.get(f'none|{model}|{effort}|{thread}')
        vs = (f'{out / n / (none[1] / none[0]) - 1:+.0%} output, {think / n / (none[2] / none[0]) - 1:+.0%} thinking'
              if exp != 'none' and none and none[1] and none[2] and n else '')
        lines.append(f'| {exp} | {model} | {effort} | {thread} | {n} | {out / max(n, 1):.0f} | {think / max(n, 1):.0f} | {vs} |')
    return lines


def write_report(fp, found, reviewed, stats, canary_lines, texts):
    new = [f for f in found if f['id'] not in reviewed]
    out = [f'# Harness audit {time.strftime("%Y-%m-%d %H:%M UTC", time.gmtime(fp["time"]))}', '',
           f'Git-ignored: it holds account data; never commit, paste or publish it. {stats}', '',
           f'## Findings ({len(new)} new of {len(found)})', '']
    for f in sorted(found, key=lambda f: (f['id'] in reviewed, f['kind'] != 'needed')):
        since = f' (since {day(f.get("time"))})' if f.get('time') != fp['time'] else ''
        out.append(f'- {"**new** " if f["id"] not in reviewed else ""}[{f["kind"]} {f["id"]}] {f["title"]}{since}')
        out += [f'  - {x}' for x in f['lines']]
    out += ['', '## Per-experiment replies', ''] + metrics_table(fp)
    inj = {}
    for k, (n, _, last) in fp['record'].get('injected', {}).items():
        typ, h = k.rsplit(' ', 1)  # the type is the client's string and may hold spaces
        inj.setdefault(typ, []).append((last, h, n))
    out += ['', '## Injected and server-delivered texts', '']
    for typ, rows in sorted(inj.items()):
        rows.sort(reverse=True)
        tag = f'{len(rows)} distinct texts (session data)' if len(rows) >= DYNAMIC else f'{len(rows)} distinct text(s)'
        out.append(f'- attachment {typ}: {tag}, newest {day(rows[0][0])}')
        out += [f'  - {texts.get(h, "")[:200]!r} ({n}x)' for _, h, n in rows[:3 if len(rows) < DYNAMIC else 1]]
    for sec in ('slot_texts', 'gb_texts'):
        for item, h in sorted(fp['snapshot'][sec].items())[:40]:
            out.append(f'- {sec} {item}: {texts.get(h, "")[:200]!r}')
    out += ['', '## Environment variable names', ''] + [f'- {k}' for k in sorted(fp['snapshot']['env_names'])]
    out += ['', '## Model catalog and aliases', ''] + [f'- {k}: {v}' for k, v in sorted(fp['snapshot']['catalog'].items())]
    out += [f'- alias {k} -> {v}' for k, v in sorted(fp['snapshot']['aliases'].items())]
    out += ['', '## Tool-output caps', '', '| tool | largest inline result | smallest saved to a file | saved | cut |', '|---|---|---|---|---|']
    out += [f'| {t} | {"" if hi is None else f"{hi / 1024:.1f} KB"} | {"" if lo is None else f"{lo / 1024:.1f} KB"} | {p} | {c} |'
            for t, (hi, lo, p, c) in sorted(fp['metrics']['tools'].items())]
    out += ['', '## Canary', ''] + (canary_lines or ['- off (enable: `git config compprog.canary true`)'])
    (AUDIT / 'report.md').write_text('\n'.join(H.redact(x) for x in out) + '\n')
    return new


@contextlib.contextmanager
def locked():
    """True while this process holds the scan lock, False when another scan holds it."""
    AUDIT.mkdir(parents=True, exist_ok=True)
    f = open(AUDIT / 'scan.lock', 'a+')  # flock ends with the process, so a killed scan leaves no stale lock
    try:
        import fcntl  # Linux and macOS, the platforms /setup-check supports
        fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
        ok = True
    except OSError:  # held by another scan (BlockingIOError) or not lockable
        ok = False
    try:
        yield ok
    finally:
        f.close()


def scan(canary_mode=None, only_due=False):
    now = time.time()
    with locked() as ok:
        if not ok:
            print('harness audit: another scan is running')
            return 0
        if only_due and not H.audit_due(now):
            return 0
        try:
            return run_scan(now, canary_mode)
        except Exception as exc:  # recorded, so the startup hook waits a week instead of failing at every start
            st = {k: v for k, v in H.load(AUDIT / 'status.json').items() if k != 'canary_retry'}  # a past retry would rerun it
            save(AUDIT / 'status.json', {**st, 'time': now, 'error': f'{type(exc).__name__}: {H.cut(exc, 200)}'})
            print(H.redact(f'harness audit: the scan failed ({type(exc).__name__}: {exc}); re-audit package P287'))
            return 1


def referenced(fp):
    """The text hashes a fingerprint's items quote: the last word of a prompt or injected item, a flag or slot text's value."""
    return ({k.rsplit(' ', 1)[-1] for sec in ('prompts', 'injected') for k in fp['record'].get(sec, {})}
            | {h for sec in ('gb_texts', 'slot_texts') for h in fp['snapshot'].get(sec, {}).values()})


def run_scan(now, canary_mode):  # canary_mode: 'force', 'skip', or None (run when enabled and due)
    conf = H.load(ROOT / '.claude/harness.json')
    cache = H.load(AUDIT / 'cache.json')
    cache.setdefault('files', {})
    texts = H.load(AUDIT / 'texts.json')
    delta, seen, changed, read_errors = empty(), set(), 0, 0
    for key, path, main_key in transcripts():
        seen.add(key)
        entry = cache['files'].setdefault(key, {})
        latch = (cache['files'].get(main_key) or {}).get('state', {}).get('latch') if main_key else None
        try:
            part = read_file(path, entry, main_key is not None, latch, texts)
        except OSError:  # deleted or unreadable since the listing: an earlier summary stays
            read_errors += 1
            if 'sum' not in entry:
                del cache['files'][key]
            continue
        if part:
            changed += 1
            merge(delta, part)
    for key, e in cache['files'].items():
        e['gone'] = key not in seen  # a deleted transcript's summary stays in the fingerprint
    snap = snapshot(texts)
    fp = fingerprint(cache, snap, now)
    hist = sorted((AUDIT / 'history').glob('*.json'))
    prev = H.load(hist[-1]) if hist else None
    prev = prev if prev and 'record' in prev else None
    ids, _ = H.allowed(ROOT)
    found = [{**f, 'time': now} for f in findings(prev, fp, delta, texts, conf, ids)]
    save(AUDIT / 'cache.json', cache)  # saved before the canary, which can run for an hour
    used = set().union(*(referenced(x) for x in (fp, prev) if x))  # texts no kept item quotes are dropped
    save(AUDIT / 'texts.json', {h: s for h, s in texts.items() if h in used})
    save(AUDIT / 'history' / f'{time.strftime("%Y%m%dT%H%M%S", time.gmtime(now))}.{int(now * 1e6) % 1000000:06d}.json', fp)
    for old in sorted((AUDIT / 'history').glob('*.json'))[:-KEEP]:
        old.unlink()
    reviewed, have = H.load(AUDIT / 'reviewed.json'), {f['id'] for f in found}
    found += [f for f in H.load(AUDIT / 'findings.json').get('findings') or ()  # unreviewed ones stay, with their time
              if isinstance(f, dict) and isinstance(f.get('id'), str) and f['id'] not in reviewed and f['id'] not in have]
    gone = sum(1 for e in cache['files'].values() if e.get('gone'))
    stats = (f'{len(seen)} transcripts ({changed} read since the last scan, {gone} deleted by Claude Code and kept here'
             f'{f", {read_errors} unreadable" if read_errors else ""}); newest record {day(fp["newest"])}'
             + ('; first scan: the baseline, no diff' if prev is None else ''))

    def publish(canary_lines, retry):  # reviewed.json reread: findings may be marked reviewed while the canary runs
        save(AUDIT / 'findings.json', {'time': now, 'findings': found})
        new = write_report(fp, found, H.load(AUDIT / 'reviewed.json'), stats, canary_lines, texts)
        save(AUDIT / 'status.json', {'time': now, 'new': len(new), 'findings': len(found), 'canary_retry': retry})
        return new
    on = canary_on()
    run = canary_mode == 'force' or (canary_mode is None and on)
    # published before the canary, so a killed canary loses no finding
    new = publish(['- running'] if run else ['- not run (plain scan, --no-canary)'] if on and canary_mode == 'skip' else [], None)
    if run:
        try:
            c_found, canary_lines, retry = canary(conf, now, canary_mode == 'force')
        except Exception as exc:  # a broken task or config: reported, and the scan still records its time
            c_found, canary_lines, retry = [], [f'- failed {day(now)}: {type(exc).__name__}: {H.cut(exc, 200)}'], None
        fresh = {f['id']: {**f, 'time': now} for f in c_found}  # found again: replaces a carried copy
        found[:] = [g for g in found if g['id'] not in fresh] + list(fresh.values())
        new = publish(canary_lines, retry)
    print(H.redact(f'harness audit: {len(new)} new finding(s) of {len(found)}; {stats}'))
    print(f'report: {AUDIT / "report.md"}')
    return 0


def canary_on():
    r = subprocess.run(['git', '-C', str(MAIN), 'config', '--get', 'compprog.canary'], capture_output=True, text=True)
    return r.returncode == 0 and r.stdout.strip().lower() in ('true', 'on', 'yes', '1')


def usage(c, now):
    """{'five_hour': %, 'seven_day': %, 'age': s, 'time': record time} from the freshest record (the status line's
    usage.json, else the client's cachedUsageUtilization); a record older than usage_max_age gives None, else a window
    whose reset time passed counts 0."""
    recs = []
    u = H.load(AUDIT / 'usage.json')
    if isinstance(u.get('time'), (int, float)):
        recs.append((u['time'], {w: (H.obj(u.get(w)).get('pct'), H.obj(u.get(w)).get('resets_at')) for w in ('five_hour', 'seven_day')}))
    g = H.obj(H.load(H.global_config()).get('cachedUsageUtilization'))
    if isinstance(g.get('fetchedAtMs'), (int, float)):
        ut = H.obj(g.get('utilization'))
        recs.append((g['fetchedAtMs'] / 1000, {w: (H.obj(ut.get(w)).get('utilization'), H.obj(ut.get(w)).get('resets_at')) for w in ('five_hour', 'seven_day')}))
    if not recs:
        return {'five_hour': None, 'seven_day': None, 'age': None, 'time': None}
    t, wins = max(recs, key=lambda r: r[0])
    out = {'age': round(now - t), 'time': t}
    for w, (pct, resets) in wins.items():
        r = epoch(resets)
        out[w] = (None if now - t > c.get('usage_max_age', 3600) else 0 if r and r <= now  # a stale record skips first
                  else pct if isinstance(pct, (int, float)) else None)
    return out


def canary(conf, now, force):
    """(findings, report lines, retry time or None): run the tasks unless a run is not due or the usage is too high."""
    c = H.obj(conf.get('canary'))
    runs_file, base_file = AUDIT / 'canary/runs.jsonl', AUDIT / 'canary/baseline.json'
    runs = []
    for x in runs_file.read_text(errors='replace').splitlines() if runs_file.is_file() else ():
        try:
            r = json.loads(x)
            runs.append((float(r['time']), int(r['passed']), len(r['results']), int(r['tokens']), float(r['cost_usd']),
                         H.obj(r.get('quota')).get('delta_five_hour')))
        except (ValueError, TypeError, KeyError):  # a damaged line is skipped
            continue
    lines = [f'- run {day(t)}: {p}/{n} passed, {tok} tokens, ${cost:.2f}, 5h {d5}' for t, p, n, tok, cost, d5 in runs[-5:]]
    if not force and runs and now - runs[-1][0] <= WEEK:
        return [], lines + ['- not due'], None
    if not c.get('tasks'):
        return [], lines + ['- no tasks in .claude/harness.json canary'], None
    before = usage(c, now)
    if before['five_hour'] is None or before['seven_day'] is None:
        return [], lines + [f'- skipped {day(now)}: usage unknown (no record within {c.get("usage_max_age", 3600)} s); retried in an hour'], now + 3600
    if before['five_hour'] > c.get('max_five_hour', 50) or before['seven_day'] > c.get('max_seven_day', 80):
        return [], lines + [f'- skipped {day(now)}: usage 5h {before["five_hour"]}%, 7d {before["seven_day"]}% above '
                            f'{c.get("max_five_hour", 50)}% or {c.get("max_seven_day", 80)}%; retried in an hour'], now + 3600
    tar = subprocess.run(['git', '-C', str(MAIN), 'archive', '--format=tar', 'HEAD'], capture_output=True, timeout=300)
    if tar.returncode:
        return [], lines + ['- failed: git archive HEAD'], None
    results = [task(t, c, tar.stdout) for t in c['tasks']]
    after = usage(c, time.time())
    tok = sum(r['tokens'] for r in results)
    rate = H.obj(H.obj(H.load(H.global_config()).get('cachedGrowthBookFeatures')).get('tengu_amber_tally'))
    per = H.obj(rate.get('creditsPerMillionTokens')).get(c.get('model'))
    credits = tok * per / 1e6 if isinstance(per, (int, float)) else None
    run = {'time': now, 'model': c.get('model'), 'effort': c.get('effort'), 'results': results,
           'passed': sum(r['passed'] for r in results), 'tokens': tok, 'cost_usd': sum(r['cost_usd'] for r in results),
           'quota': {'before': before, 'after': after,
                     # only a usage record taken after the tasks started measures them
                     'delta_five_hour': (after['five_hour'] - before['five_hour'])
                     if None not in (after['five_hour'], before['five_hour']) and (after['time'] or 0) > now else None,
                     'credits_estimate': credits,
                     'five_hour_share_estimate': {k: credits / v for k, v in H.obj(rate.get('fiveHourCredits')).items() if isinstance(v, (int, float)) and v} if credits else None}}
    runs_file.parent.mkdir(parents=True, exist_ok=True)
    with open(runs_file, 'a') as f:
        f.write(json.dumps(run, sort_keys=True) + '\n')
    base = H.load(base_file)
    if not base and all(r['ran'] and r.get('claude_rc') == 0 for r in results):  # a failed claude run is no baseline
        save(base_file, run)
    lines.append(f'- run {day(now)}: {run["passed"]}/{len(results)} passed, {tok} tokens, ${run["cost_usd"]:.2f}, '
                 f'5h {run["quota"]["delta_five_hour"]}, credits estimate {credits}')
    found = []
    if base:
        b = {r['id']: r for r in base['results']}
        worse = [r['id'] for r in results if not r['passed'] and b.get(r['id'], {}).get('passed')]
        if run['passed'] / len(results) < base['passed'] / max(len(base['results']), 1) or worse:
            found.append(('canary', f'canary pass rate {run["passed"]}/{len(results)}, baseline {base["passed"]}/{len(base["results"])}', [f'failed now: {x}' for x in worse]))
        if base['tokens'] and tok > c.get('tolerance', 1.5) * base['tokens']:
            found.append(('canary', f'canary tokens {tok}, {tok / base["tokens"]:.2f}x the baseline {base["tokens"]}', []))
    return [{'id': digest(k + t + '\n'.join(ls)), 'kind': k, 'title': t, 'lines': ls} for k, t, ls in found], lines, None


def task(t, c, archive):
    """One canary task in a scratch copy of HEAD: its file removed, claude asked to write it, its test as the oracle."""
    d = Path(tempfile.mkdtemp(prefix='harness-canary-'))
    res = {'id': H.obj(t).get('id'), 'passed': False, 'ran': False, 'tokens': 0, 'cost_usd': 0.0}
    try:
        with tarfile.open(fileobj=io.BytesIO(archive)) as tf:
            tf.extractall(d, filter='data')
        git = lambda *a: subprocess.run(['git', '-C', str(d), '-c', 'user.name=canary', '-c', 'user.email=canary@localhost',
                                         '-c', 'commit.gpgsign=false', '-c', 'core.hooksPath=/dev/null', *a],
                                        capture_output=True, text=True, timeout=300)
        target, script = (d / t['remove']).resolve(), (d / t['test'][0]).resolve()
        if not (target.is_relative_to(d.resolve()) and script.is_relative_to(d.resolve())):
            res['error'] = 'a task path leaves the scratch copy'
            return res
        if not target.is_file():  # a task naming a file HEAD lacks is not run, so it never spends usage
            res['error'] = f'{t["remove"]} is not in HEAD'
            return res
        target.unlink()  # before the base commit, so no git command in the copy can restore it
        for a in (('init', '-q'), ('add', '-A'), ('commit', '-qm', 'canary base')):  # a clone of its own: Claude's hooks run as in one
            if git(*a).returncode:
                res['error'] = f'git {a[0]} failed in the scratch copy'
                return res
        cmd = ['claude', '-p', PROMPT.format(remove=t['remove'], test=' '.join(f"'{x}'" if ' ' in x else x for x in t['test'])),
               '--model', c['model'], '--effort', c['effort'], '--output-format', 'json', '--permission-mode', 'acceptEdits',
               '--no-session-persistence', '--max-budget-usd', str(c.get('budget_usd', 3))]
        t0 = time.time()
        try:
            r = subprocess.run(cmd, cwd=d, capture_output=True, text=True, timeout=c.get('timeout', 1200),
                               env={**os.environ, 'CP_HARNESS_AUDIT_CHILD': '1'})
        except (OSError, subprocess.TimeoutExpired) as exc:
            res['error'] = type(exc).__name__
            return res
        res['ran'], res['seconds'], res['claude_rc'] = True, round(time.time() - t0), r.returncode
        try:
            out = H.obj(next((json.loads(x) for x in reversed(r.stdout.splitlines()) if x.startswith('{')), {}))
        except ValueError:  # damaged output: the run counts, its tokens are unknown
            out, res['error'] = {}, 'claude output is not JSON'
        u = H.obj(out.get('usage'))
        res['usage'] = {k: u.get(k) for k in ('input_tokens', 'cache_creation_input_tokens', 'cache_read_input_tokens', 'output_tokens')}
        res['tokens'] = sum(v for v in res['usage'].values() if isinstance(v, int))
        res['cost_usd'] = out.get('total_cost_usd') if isinstance(out.get('total_cost_usd'), (int, float)) else 0.0
        res['turns'] = out.get('num_turns')
        status = git('status', '--porcelain', '--untracked-files=no', '-z').stdout.split('\0')
        edited = sorted(x[3:] for x in status if x[3:] and x[3:] != t['remove'])
        res['edited'] = edited[:5]
        test = subprocess.run([sys.executable, *t['test']], cwd=d, capture_output=True, text=True, timeout=c.get('timeout', 1200))
        res['test_rc'] = test.returncode
        res['passed'] = r.returncode == 0 and target.is_file() and not edited and test.returncode == 0
        return res
    except Exception as exc:  # one broken task (bad entry, test timeout) fails alone; the run is still recorded
        res['error'] = f'{type(exc).__name__}: {H.cut(exc, 200)}'
        return res
    finally:
        shutil.rmtree(d, ignore_errors=True)


def report():
    f = H.load(AUDIT / 'findings.json')
    if not f:
        print('harness audit: no scan yet; run `harness_audit.py scan`')
        return 0
    reviewed = H.load(AUDIT / 'reviewed.json')
    new = [x for x in f['findings'] if x['id'] not in reviewed]
    print(f'report: {AUDIT / "report.md"} ({time.strftime("%Y-%m-%d", time.gmtime(f["time"]))}); {len(new)} new finding(s)')
    for x in new:
        print(H.redact(f'- [{x["kind"]} {x["id"]}] {x["title"]}'))
    return 0


def mark_reviewed():
    f = H.load(AUDIT / 'findings.json')
    reviewed = H.load(AUDIT / 'reviewed.json')
    reviewed.update({x['id']: time.time() for x in f.get('findings', ()) if x['id'] not in reviewed})
    save(AUDIT / 'reviewed.json', reviewed)
    st = H.load(AUDIT / 'status.json')
    if st:
        save(AUDIT / 'status.json', {**st, 'new': 0})
    print(f'harness audit: {len(f.get("findings", ()))} finding(s) marked reviewed')
    return 0


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('command', choices=('scan', 'report', 'reviewed'))
    ap.add_argument('--due', action='store_true', help='scan only when the weekly scan or a canary retry is due')
    g = ap.add_mutually_exclusive_group()
    g.add_argument('--canary', action='store_true', help='run the canary now, enabled or not')
    g.add_argument('--no-canary', action='store_true', help='never run the canary (the skill\'s plain scan)')
    a = ap.parse_args(argv)
    if a.command == 'report':
        return report()
    if a.command == 'reviewed':
        return mark_reviewed()
    return scan('force' if a.canary else 'skip' if a.no_canary else None, a.due)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
