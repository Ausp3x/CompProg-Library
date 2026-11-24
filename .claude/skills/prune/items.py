#!/usr/bin/env python3
"""Print one package's section of duplicates.md and the recorded decisions that name it, fitted to the room /prune's
SKILL.md leaves its `!` line after a compaction (plan.fit; whole lines, then a pointer to duplicates.md)."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
FILE = ROOT / '00-Guidelines/23-Sweep Ledgers/duplicates.md'
sys.path.insert(0, str(ROOT / '00-Guidelines/13-Plan'))
import plan  # noqa: E402


def main():
    pid = sys.argv[1] if len(sys.argv) > 1 else ''
    if not re.fullmatch(r'P\d{3}', pid):
        sys.exit(f'usage: items.py Pxxx (got {pid!r})')
    text = FILE.read_text()
    section = re.search(rf'^## {pid} .*?(?=^## |\Z)', text, re.M | re.S)
    out = (section.group(0).rstrip() if section else f'No package section for {pid}.').split('\n')
    decisions = re.search(r'^## Recorded Decisions\n(.*?)(?=^## |\Z)', text, re.M | re.S)
    blocks = re.findall(r'^- .*?(?=^- |^#|^\||\Z)|^\|[^\n]*', decisions.group(1), re.M | re.S) if decisions else []
    named = [b.rstrip() for b in blocks if re.search(rf'\b{pid}\b', b)]
    out += ['', f'Recorded Decisions naming {pid}:', *('\n'.join(named).split('\n') if named else ['none'])]
    rest = f'`## {pid}` and `## Recorded Decisions` in {FILE.relative_to(ROOT)} hold them'
    print('\n'.join(plan.fit(out, plan.room_of(Path(__file__).with_name('SKILL.md')), rest,
                             lambda line: line.startswith(('## ', 'Recorded Decisions naming')))))


if __name__ == '__main__':
    main()
