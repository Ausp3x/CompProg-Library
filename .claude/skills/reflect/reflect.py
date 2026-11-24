#!/usr/bin/env python3
"""Inputs for /reflect (P289 SUP34; evidence: 00-Guidelines/13-Plan/Docs/00-harness-review.md, Reflect).

  reflect.py list [--since YYYY-MM-DD]   write what the user typed, answered or rejected in this clone's main transcripts
                                         since the last recorded run (else 30 days back) into chunk files for the
                                         reading agents, and print their paths
  reflect.py done                        record the time `list` started as the last run, once the user has decided every row

Reads the transcript folders the weekly audit reads (harness_audit.project_folders) and writes only under the
git-ignored .claude/reflect/ of the main checkout, since the chunks quote the user's messages. Each item carries its
time, session, kind and the end of the reply before it, so a reader can tell a correction from an instruction."""
import json
import re
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'harness-audit'))
import harness_audit as A  # noqa: E402

OUT = A.MAIN / '.claude/reflect'
CHUNK = 60_000  # characters per chunk file: one reading agent's share
CONTEXT = 400  # characters of the reply before an item
TEXT = 2_000  # characters kept of one item
SKIP = re.compile(r'^\s*(<(local-command|bash-|task-notification|command-name|system-reminder)|Caveat:)')
REJECTED = "The user doesn't want to proceed with this tool use."


def epoch(s):
    t = A.epoch(s)
    return t if isinstance(t, (int, float)) else None


def items(path, since):
    """[(time, kind, text, reply before)] of one main transcript after `since`."""
    out, reply, asks = [], '', set()
    for line in open(path, errors='replace'):
        try:
            e = json.loads(line)
        except ValueError:
            continue
        if not isinstance(e, dict) or e.get('isMeta') or e.get('isCompactSummary') or e.get('isSidechain'):
            continue
        t, m = epoch(e.get('timestamp')), A.H.obj(e.get('message'))
        content = m.get('content')
        if e.get('type') == 'assistant' and isinstance(content, list):
            for b in content:
                if isinstance(b, dict) and b.get('type') == 'text' and b.get('text', '').strip():
                    reply = b['text']
                elif isinstance(b, dict) and b.get('type') == 'tool_use' and b.get('name') == 'AskUserQuestion':
                    asks.add(b.get('id'))
                    reply = json.dumps(A.H.obj(b.get('input')).get('questions'), ensure_ascii=False)
            continue
        if e.get('type') != 'user' or t is None or t <= since:
            continue
        if isinstance(content, str):
            g = re.search(r'<command-name>(/[\w:-]+)</command-name>.*?<command-args>(.*?)</command-args>', content, re.S)
            text, kind = (f'{g.group(1)} {g.group(2).strip()}', 'command') if g else (content, 'typed')
            if g and not g.group(2).strip() or not g and SKIP.match(content):
                continue
            out.append((t, kind, text, reply))
        elif isinstance(content, list):
            for b in content:
                if not isinstance(b, dict):
                    continue
                body = b.get('content')
                body = body if isinstance(body, str) else '\n'.join(x.get('text', '') for x in body if isinstance(x, dict)) \
                    if isinstance(body, list) else ''
                if b.get('type') == 'text' and b.get('text', '').strip() and not SKIP.match(b['text']):
                    out.append((t, 'typed', b['text'], reply))
                elif b.get('type') == 'tool_result' and b.get('tool_use_id') in asks:
                    out.append((t, 'answer', body, reply))
                elif b.get('type') == 'tool_result' and body.startswith(REJECTED):
                    out.append((t, 'rejected', body, reply))
    return out


def stamp(t):
    return time.strftime('%Y-%m-%d %H:%M', time.localtime(t))


def collect(since):
    """[(session, [(time, kind, text, reply)])] of every main transcript with an item after `since`, oldest first."""
    found = []
    for _, path, main in A.transcripts():
        if main is None and path.stat().st_mtime > since:
            got = items(path, since)
            if got:
                found.append((path.stem, got))
    return sorted(found, key=lambda s: s[1][0][0])


def list_(since_arg):
    now = time.time()
    last = A.H.load(OUT / 'last.json').get('time')
    since = (time.mktime(time.strptime(since_arg, '%Y-%m-%d')) if since_arg
             else last if isinstance(last, (int, float)) else now - 30 * 86400)
    sessions = collect(since)
    OUT.mkdir(parents=True, exist_ok=True)
    for old in OUT.glob('chunk-*.txt'):
        old.unlink()
    chunks, cur, size = [], [], 0
    for sid, got in sessions:
        head, first = f'## session {sid}\n', True
        for t, kind, text, reply in got:
            item = f'- {stamp(t)} {kind}: {text.strip()[:TEXT]}\n' + (f'  before: …{reply.strip()[-CONTEXT:]}\n' if reply.strip() else '')
            if cur and size + len(head) + len(item) > CHUNK:  # a new chunk repeats its session's heading
                chunks.append(cur)
                cur, size, first = [], 0, True
            if first:
                cur.append(head)
                size, first = size + len(head), False
            cur.append(item)
            size += len(item)
    if cur:
        chunks.append(cur)
    paths = []
    for i, c in enumerate(chunks, 1):
        p = OUT / f'chunk-{i:02d}.txt'
        p.write_text(''.join(c))
        paths.append(p)
    A.save(OUT / 'pending.json', {'time': now, 'since': since, 'chunks': len(paths)})
    n = sum(len(g) for _, g in sessions)
    print(f'reflect: {n} item(s) in {len(sessions)} session(s) since {stamp(since)}; {len(paths)} chunk file(s):')
    for p in paths:
        print(f'- {p}')
    return 0


def done():
    p = A.H.load(OUT / 'pending.json')
    if not isinstance(p.get('time'), (int, float)):
        print('reflect: no pending run; run `reflect.py list` first')
        return 1
    A.save(OUT / 'last.json', {'time': p['time']})
    (OUT / 'pending.json').unlink()
    for old in OUT.glob('chunk-*.txt'):
        old.unlink()
    print(f'reflect: last run recorded as {stamp(p["time"])}')
    return 0


def main(argv):
    if argv[:1] == ['done'] and len(argv) == 1:
        return done()
    if argv[:1] == ['list'] and (len(argv) == 1 or len(argv) == 3 and argv[1] == '--since'):
        return list_(argv[2] if len(argv) == 3 else None)
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
