#!/usr/bin/env python3
"""Explicitly regenerate or check 99-Workspace/template.cpp."""

from __future__ import annotations

import argparse
import difflib
import os
import re
import sys
import tempfile
from pathlib import Path


LIBRARY = Path(__file__).resolve().parent.parent
TARGET = LIBRARY / "99-Workspace" / "template.cpp"
LOCAL_INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"\s*$')
PRAGMA_ONCE = re.compile(r"^\s*#\s*pragma\s+once\s*$")
MOTTO = "// 知彼知己，百战不殆"
DRIVER = """\
void solve(int t) {
    // trace(to_string(t));

    return;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int t = 1;
    cin >> t;
    for (int i = 1; i <= t; i++) {
        solve(i);
    }

    return 0;
}
"""


def header(name: str) -> str:
    path = LIBRARY / "01-Core" / name
    lines = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if PRAGMA_ONCE.fullmatch(line):
            continue
        match = LOCAL_INCLUDE.fullmatch(line)
        if match:
            if name == "02-debug.hpp" and match.group(1) in ("01-template.hpp", "../01-Core/01-template.hpp"):
                continue
            raise ValueError(f"unexpanded local include in {path}: {line}")
        if line.strip().startswith("//") and line.strip() != MOTTO:
            continue
        lines.append(line)
    return "\n".join(lines).strip() + "\n"


def generate() -> str:
    text = header("01-template.hpp") + "\n" + header("02-debug.hpp") + "\n" + DRIVER
    if not text.startswith(MOTTO + "\n"):
        raise ValueError("01-template.hpp must start with the motto line")
    return text


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="show a diff and exit nonzero if the snapshot is stale")
    args = parser.parse_args()
    try:
        expected = generate()
        actual = TARGET.read_text(encoding="utf-8") if TARGET.exists() else ""
        if args.check:
            if actual == expected:
                print(f"Current: {TARGET}")
                return 0
            sys.stdout.writelines(difflib.unified_diff(
                actual.splitlines(keepends=True), expected.splitlines(keepends=True),
                fromfile=str(TARGET), tofile="regenerated template.cpp"))
            return 1
        TARGET.parent.mkdir(parents=True, exist_ok=True)
        fd, temp = tempfile.mkstemp(prefix=".template-", dir=TARGET.parent)
        try:
            with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as file:
                file.write(expected)
            os.replace(temp, TARGET)
        finally:
            if os.path.exists(temp):
                os.unlink(temp)
        print(f"Generated: {TARGET}")
        return 0
    except (OSError, ValueError) as exc:
        print(f"Workspace generation failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
