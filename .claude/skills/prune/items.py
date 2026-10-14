#!/usr/bin/env python3
"""Print one package's section of duplicates.md and the recorded decisions that name it."""
import re
import sys
from pathlib import Path

FILE = Path(__file__).resolve().parents[3] / '00-Guidelines/23-Sweep Ledgers/duplicates.md'


def main():
    pid = sys.argv[1] if len(sys.argv) > 1 else ''
    if not re.fullmatch(r'P\d{3}', pid):
        sys.exit(f'usage: items.py Pxxx (got {pid!r})')
    text = FILE.read_text()
    section = re.search(rf'^## {pid} .*?(?=^## |\Z)', text, re.M | re.S)
    print(section.group(0).rstrip() if section else f'No package section for {pid}.')
    decisions = re.search(r'^## Recorded decisions\n(.*?)(?=^## |\Z)', text, re.M | re.S)
    blocks = re.findall(r'^- .*?(?=^- |^#|^\||\Z)|^\|[^\n]*', decisions.group(1), re.M | re.S) if decisions else []
    named = [b.rstrip() for b in blocks if re.search(rf'\b{pid}\b', b)]
    print(f'\nRecorded decisions naming {pid}:')
    print('\n'.join(named) if named else 'none')


if __name__ == '__main__':
    main()
