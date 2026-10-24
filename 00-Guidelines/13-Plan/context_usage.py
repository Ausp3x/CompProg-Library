#!/usr/bin/env python3
"""Token and context usage of past Claude Code sessions in this repository, read from the local transcripts.

Usage: context_usage.py [--since YYYY-MM-DD] [--skill /package] [--top 15] [--windows 600000,400000,250000] [--split]
Per session: model calls, peak context, input tokens processed (every call re-reads its whole context), output
tokens, compactions, and subagents with their input. The replay estimates the input a session would have
processed under an auto-compact window: the context grows by the same per-call steps and drops to 60k tokens
whenever it would exceed the window. --split groups the calls by model and effort (subagents also by type) and
summarizes finish-log.tsv by model and effort, the measure for the effort setting. The model ignores the session's own drops (compactions, rewinds) and the files
re-read after a compaction, so each window is compared with the same model at the default window (DEFAULT, about
967k on a 1M-context model), never with the actual input. The totals
include the clangd sign-conversion diagnostics attached to the sessions.
"""
import argparse
import collections
import csv
import json
import re
import subprocess
from pathlib import Path

RESET = 60_000
DEFAULT = 967_000
IGNORED = ('/effort', '/model', '/clear', '/compact', '/context', '/cost', '/config')


def scan(path, seen):
    """Usage of one transcript; `seen` holds entry and message ids already counted, since a resumed or forked
    transcript repeats its parent's entries."""
    ctxs, split = [], collections.defaultdict(lambda: [0, 0, 0, 0])  # (model, effort): calls, input, writes, output
    mine = {}  # message id -> (split key, output counted): streamed entries repeat a message with growing output
    out = compactions = signedness = 0
    cmd = date = None
    for line in open(path, errors='replace'):
        try:
            e = json.loads(line)
        except ValueError:
            continue
        date = date or (e.get('timestamp') or '')[:10] or None
        kind, m, uid = e.get('type'), e.get('message') or {}, e.get('uuid')
        if kind == 'user' and cmd is None and isinstance(m.get('content'), str):  # read before deduplication
            g = re.search(r'<command-name>(/[\w-]+)</command-name>.*?<command-args>(.*?)</command-args>', m['content'], re.S)
            if g and g.group(1) not in IGNORED:
                cmd = ' '.join([g.group(1), *g.group(2).split()[:1]])
        if uid in seen:
            continue
        if uid:
            seen.add(uid)
        if kind == 'system' and e.get('subtype') == 'compact_boundary':
            compactions += 1
        if kind == 'attachment' and (e.get('attachment') or {}).get('type') == 'diagnostics':
            signedness += json.dumps(e['attachment']).count('changes signedness')
        u = m.get('usage') if kind == 'assistant' else None
        if u and m.get('id') in mine:
            key, counted = mine[m['id']]
            extra = max(0, u.get('output_tokens', 0) - counted)
            out, split[key][3], mine[m['id']] = out + extra, split[key][3] + extra, (key, counted + extra)
        elif u and m.get('id') not in seen:
            seen.add(m.get('id'))
            ctxs.append(u.get('input_tokens', 0) + u.get('cache_read_input_tokens', 0) + u.get('cache_creation_input_tokens', 0))
            out += u.get('output_tokens', 0)
            key = (str(m.get('model', '?')).removeprefix('claude-'), e.get('effort') or '-')
            mine[m.get('id')], row = (key, u.get('output_tokens', 0)), split[key]
            for i, x in enumerate((1, ctxs[-1], u.get('cache_creation_input_tokens', 0), u.get('output_tokens', 0))):
                row[i] += x
    return dict(cmd=cmd or '(none)', date=date or '?', ctxs=ctxs, out=out, compactions=compactions, signedness=signedness, split=split)


def agent_type(path):
    try:
        return json.loads(path.with_suffix('.meta.json').read_text()).get('agentType', '?')
    except (OSError, ValueError):
        return '?'


def print_split(sessions):
    main, subs = collections.defaultdict(lambda: [0, 0, 0, 0]), collections.defaultdict(lambda: [0, 0, 0, 0])
    for s in sessions:
        for target, items in ((main, [('', s['split'])]), (subs, s['sub_splits'])):
            for kind, split in items:
                for (model, effort), row in split.items():
                    target[(kind, model, effort)] = [a + b for a, b in zip(target[(kind, model, effort)], row)]
    for title, table in (('main sessions', main), ('subagents', subs)):
        print(f'\nby {"agent type, " if table is subs else ""}model and effort ({title}): calls, input, cache writes, output')
        for (kind, model, effort), (calls, inp, writes, out) in sorted(table.items(), key=lambda kv: -kv[1][1]):
            print(f'  {kind[:18]:18} {model[:16]:16} {effort:6} {calls:6} {inp / 1e6:7.0f}M {writes / 1e6:6.1f}M {out / 1e6:6.2f}M')
    log = Path(__file__).resolve().parent / 'finish-log.tsv'
    rows = list(csv.DictReader(open(log), delimiter='\t')) if log.is_file() else []
    groups = collections.defaultdict(list)
    for r in rows:
        groups[(r['model'], r['effort'])].append(r)
    print('\nfinish-log.tsv by model and effort: landings, mean review findings, defects fixed, decisions asked')
    for (model, effort), rs in sorted(groups.items()):
        mean = lambda k: sum(float(r[k] or 0) for r in rs) / len(rs)
        print(f'  {model[:16]:16} {effort:6} {len(rs):4} {mean("review_findings"):6.1f} {mean("defects_fixed"):6.1f} {mean("decisions"):6.1f}')
    if not groups:
        print('  (no landings logged yet)')


def replay(ctxs, window):
    """(input processed, compactions) if the context had been compacted to RESET whenever it exceeded window."""
    total = resets = 0
    cur = prev = ctxs[0] if ctxs else 0
    for x in ctxs:
        cur += max(0, x - prev)
        prev = x
        if cur > window:
            cur, resets = RESET, resets + 1
        total += cur
    return total, resets


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--since', default='')
    parser.add_argument('--skill', default='/package', help="first command of the session; '' for all sessions")
    parser.add_argument('--top', type=int, default=15)
    parser.add_argument('--windows', default='600000,400000,250000')
    parser.add_argument('--split', action='store_true', help='also group calls by model and effort, and summarize finish-log.tsv')
    args = parser.parse_args()
    listing = subprocess.run(['git', 'worktree', 'list', '--porcelain'], capture_output=True, text=True, check=True).stdout
    main_root = listing.split('\n', 1)[0].split(' ', 1)[1]
    projects = Path.home() / '.claude/projects'
    files = sorted((f for d in projects.glob(re.sub(r'[^A-Za-z0-9]', '-', main_root) + '*') for f in d.glob('*.jsonl')),
                   key=lambda f: -f.stat().st_size)  # a copy is smaller than or equal to its source
    sessions, seen = [], set()
    for f in files:
        s = scan(f, seen)
        if s['ctxs'] and s['date'] >= args.since and s['cmd'].startswith(args.skill):
            paths = sorted((f.parent / f.stem / 'subagents').glob('*.jsonl'))
            subs = [scan(x, seen) for x in paths]
            s.update(sub_splits=[(agent_type(x), sub['split']) for x, sub in zip(paths, subs)], subs=len(subs), sub_input=sum(sum(x['ctxs']) for x in subs), signedness=s['signedness'] + sum(x['signedness'] for x in subs))
            sessions.append(s)
    sessions.sort(key=lambda s: -sum(s['ctxs']))
    windows = [int(w) for w in args.windows.split(',') if w]
    print(f'{len(sessions)} sessions (first command {args.skill or "any"}, since {args.since or "the start"}); tokens in M = millions')
    print(f'{"date":10} {"command":16} {"calls":>5} {"peak":>6} {"input":>7} {"output":>6} {"cmp":>3} {"subs":>4} {"sub in":>7}  replay at {DEFAULT // 1000}k, then at '
          + ', '.join(f'{w // 1000}k' for w in windows) + ' (compactions, saving)')
    for s in sessions[:args.top]:
        base = replay(s['ctxs'], DEFAULT)[0]
        replays = f'{base / 1e6:6.0f}M ' + ' '.join(f'{t / 1e6:5.0f}M ({n}, {100 - 100 * t / base:3.0f}%)' for t, n in (replay(s['ctxs'], w) for w in windows))
        print(f'{s["date"]:10} {s["cmd"][:16]:16} {len(s["ctxs"]):5} {max(s["ctxs"], default=0) / 1e3:5.0f}k {sum(s["ctxs"]) / 1e6:6.0f}M '
              f'{s["out"] / 1e3:5.0f}k {s["compactions"]:3} {s["subs"]:4} {s["sub_input"] / 1e6:6.0f}M  {replays}')
    print(f'totals: main input {sum(sum(s["ctxs"]) for s in sessions) / 1e6:.0f}M, subagent input {sum(s["sub_input"] for s in sessions) / 1e6:.0f}M, '
          f'compactions {sum(s["compactions"] for s in sessions)}, sign-conversion diagnostics {sum(s["signedness"] for s in sessions)}')
    if args.split:
        print_split(sessions)


if __name__ == '__main__':
    main()
