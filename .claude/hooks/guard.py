#!/usr/bin/env python3
"""PreToolUse guard: protect archives and generated files, and block context-blowing reads."""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BIG = 48 * 1024


def deny(reason):
    print(json.dumps({'hookSpecificOutput': {'hookEventName': 'PreToolUse', 'permissionDecision': 'deny',
                                             'permissionDecisionReason': reason}}))


def main():
    data = json.load(sys.stdin)
    tool, inp = data.get('tool_name'), data.get('tool_input') or {}
    raw = inp.get('file_path') or inp.get('notebook_path')
    if not raw:
        return
    path = Path(raw)
    path = path if path.is_absolute() else ROOT / path
    try:
        rel = path.resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return
    if tool in ('Edit', 'Write', 'NotebookEdit'):
        if (rel.startswith('OLD/') and rel != 'OLD/00-index.md') or '/97-Legacy/' in rel:
            return deny('Archived material under OLD/ and 97-Legacy/ is immutable; account for its features in the inventory and notes instead.')
        if rel == '99-Workspace/template.cpp':
            return deny("template.cpp is generated: edit 01-Core/01-template.hpp or 02-debug.hpp, then run python3 '97-Online Testing/03-workspace.py'.")
        return
    if tool == 'Read':
        if rel.startswith('95-Resources/') and rel.endswith('.pdf'):
            return deny('Reference PDFs are not read whole; use the pages parameter for at most a few pages, or rely on the online catalogs in 00-Guidelines/09-sources.md.')
        if rel.startswith('00-Guidelines/Ledgers/') and rel.endswith('.json'):
            return deny('Machine ledgers are not read whole; query them with rg or a short python3 snippet.')
        if path.is_file() and path.stat().st_size > BIG and not inp.get('limit'):
            return deny(f'{rel} is {path.stat().st_size // 1024} KB; read a slice with offset/limit or search it with rg.')


if __name__ == '__main__':
    try:
        main()
    except Exception:  # Never break a tool call because the guard failed.
        pass
    sys.exit(0)
