#!/usr/bin/env python3
"""Write the omission-sweep scope as small chunk files and print a summary.

usage: scope.py OUTDIR [folder prefixes | header or module paths]
Chunks hold at most four rows of one folder; each file lists header, tier, status,
owning package and operations, and families.md lists every folder swept whole.
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / '00-Guidelines/13-Plan'))
import plan  # noqa: E402

CHUNK = 4


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    out, wanted = Path(sys.argv[1]), sys.argv[2:]
    prefixes = [w for w in wanted if re.fullmatch(r'0[1-8]', w)]
    paths = [w for w in wanted if w not in prefixes]
    if not wanted:
        prefixes = ['%02d' % i for i in range(1, 9)]
    batches, packages = plan.load(ROOT)
    owner = {t: p for p in packages['packages'] for b in p['batches']
             for t in next(x for x in batches['batches'] if x['id'] == b)['targets']}
    inv = plan.inventories(ROOT)
    rows = [t for t in inv if t[:2] in prefixes or t in paths]
    missing = [p for p in paths if p not in inv]
    if missing:
        sys.exit(f'not an inventory target: {missing}')
    out.mkdir(parents=True, exist_ok=True)
    for f in out.glob('*.md'):
        f.unlink()
    chunks, by_folder = [], {}
    for t in rows:
        by_folder.setdefault(t.split('/')[0], []).append(t)
    for folder, targets in by_folder.items():
        for i in range(0, len(targets), CHUNK):
            chunks.append((folder, targets[i:i + CHUNK]))
    for n, (folder, targets) in enumerate(chunks):
        lines = [f'# Chunk {n} — {folder}', '']
        for t in targets:
            cells, tier = inv[t]
            p = owner.get(t)
            lines += [f'## `{t}`', f'- Tier: {tier}',
                      f'- Owner: {p["id"]} ({p["status"]}) — {p["label"]}' if p else '- Owner: none',
                      f'- Status: {cells[-1]}', f'- Operations: {cells[-2] if len(cells) > 1 else ""}', '']
        (out / f'chunk-{n:03d}.md').write_text('\n'.join(lines))
    whole = [f for f in by_folder if f[:2] in prefixes]
    (out / 'families.md').write_text('\n'.join(['# Folders swept whole', ''] + [
        f'- `{f}`: ' + ', '.join(f'`{t.split("/")[1]}`' for t in inv if t.startswith(f + '/')) for f in whole]) + '\n')
    manifest = {'dir': str(out), 'chunks': len(chunks), 'folders': whole}
    (out / 'manifest.json').write_text(json.dumps(manifest))
    print(json.dumps(manifest))
    for f, ts in by_folder.items():
        print(f'{f}: {len(ts)} rows')
    print(f'{len(chunks)} chunks, {len(whole)} family sweeps')


if __name__ == '__main__':
    main()
