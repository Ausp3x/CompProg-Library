#!/usr/bin/env python3
"""Bundle C++ judge sources into standalone, manifest-owned submissions."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import stat
import subprocess
import sys
import tempfile
from pathlib import Path, PurePosixPath


ROOT = Path(__file__).resolve().parent
SOURCES = ROOT / "src"
EXPANDED = ROOT / "expanded"
MANIFEST = ROOT / "02-expansion.json"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def output_name(source: str) -> str:
    return str(PurePosixPath("expanded") / PurePosixPath(*PurePosixPath(source).parts[1:]).with_suffix(".expanded.cpp"))


def safe_source_name(name: object) -> bool:
    if not isinstance(name, str) or "\\" in name:
        return False
    path = PurePosixPath(name)
    return (bool(name) and not path.is_absolute() and len(path.parts) >= 2
            and path.parts[0] == "src" and all(p not in (".", "..") for p in path.parts)
            and str(path) == name and path.suffix == ".cpp")


def check_path(path: Path, expect_file: bool = False) -> None:
    """Reject symlinks in generated paths, including intermediate directories."""
    current = ROOT
    for part in path.relative_to(ROOT).parts:
        current = current / part
        try:
            mode = current.lstat().st_mode
        except FileNotFoundError:
            continue
        if stat.S_ISLNK(mode):
            raise ValueError(f"refusing symlink: {current}")
        if current != path and not stat.S_ISDIR(mode):
            raise ValueError(f"expected directory: {current}")
        if current == path and expect_file and not stat.S_ISREG(mode):
            raise ValueError(f"expected regular file: {current}")


def load_manifest() -> dict[str, dict[str, str]]:
    check_path(MANIFEST, expect_file=True)
    if not MANIFEST.exists():
        return {}
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if not isinstance(data, dict) or data.get("version") != 1 or not isinstance(data.get("outputs"), dict):
        raise ValueError("invalid expansion manifest format")
    entries = data["outputs"]
    for source, record in entries.items():
        if not safe_source_name(source) or not isinstance(record, dict):
            raise ValueError(f"unsafe manifest entry: {source!r}")
        output = record.get("output")
        sha256 = record.get("sha256")
        if output != output_name(source) or not isinstance(sha256, str) or len(sha256) != 64 or any(c not in "0123456789abcdef" for c in sha256):
            raise ValueError(f"unsafe manifest entry: {source!r}")
        check_path(ROOT / output, expect_file=True)
    return entries


def discover() -> tuple[list[tuple[str, Path]], int]:
    check_path(SOURCES)
    if not SOURCES.is_dir():
        raise ValueError(f"source directory missing: {SOURCES}")
    found: list[tuple[str, Path]] = []
    python_count = 0
    def walk_error(error: OSError) -> None:
        raise error

    for folder, dirs, files in os.walk(SOURCES, followlinks=False, onerror=walk_error):
        base = Path(folder)
        for name in dirs + files:
            if (base / name).is_symlink():
                raise ValueError(f"refusing symlink in source tree: {base / name}")
        dirs.sort()
        for name in sorted(files):
            path = base / name
            if name.endswith(".py"):
                python_count += 1
            if name.endswith(".cpp") and not name.endswith(".expanded.cpp"):
                if not path.is_file():
                    raise ValueError(f"expected regular source file: {path}")
                relative = path.relative_to(ROOT).as_posix()
                found.append((relative, path))
    return found, python_count


def write_atomic(path: Path, data: bytes) -> None:
    check_path(path, expect_file=True)
    path.parent.mkdir(parents=True, exist_ok=True)
    check_path(path, expect_file=True)
    fd, temp = tempfile.mkstemp(prefix=".expansion-", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as file:
            file.write(data)
        os.replace(temp, path)
    finally:
        if os.path.exists(temp):
            os.unlink(temp)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bundler", default="oj-bundle", help="oj-bundle executable (default: oj-bundle)")
    cleanup = parser.add_mutually_exclusive_group()
    cleanup.add_argument("--clean", dest="clean", action="store_true", default=True, help="remove orphaned owned outputs (default)")
    cleanup.add_argument("--noclean", dest="clean", action="store_false", help="retain orphaned owned outputs")
    parser.add_argument("--timeout", type=float, default=120, help="seconds allowed per bundler process")
    args = parser.parse_args()
    if args.timeout <= 0: parser.error("--timeout must be positive")

    try:
        entries = load_manifest()
        sources, python_count = discover()
        check_path(EXPANDED)
        bundler = args.bundler
        if os.sep in bundler:
            bundler = str(Path(bundler).absolute())
        if not shutil.which(bundler):
            raise ValueError(f"{args.bundler!r} unavailable; install with: python -m pip install online-judge-verify-helper")

        # Validate all destinations before running the external tool or changing any file.
        for source, _ in sources:
            target = ROOT / output_name(source)
            check_path(target, expect_file=True)
            if target.exists():
                prior = entries.get(source)
                if prior is None or digest(target.read_bytes()) != prior["sha256"]:
                    raise ValueError(f"refusing to overwrite unowned or modified output: {target}")

        bundled: dict[str, bytes] = {}
        failures = 0
        for source, path in sources:
            try:
                result = subprocess.run([bundler, str(path)], cwd=ROOT, capture_output=True, timeout=args.timeout)
            except subprocess.TimeoutExpired:
                failures += 1
                print(f"FAILED {source}: bundler exceeded {args.timeout}s", file=sys.stderr)
                continue
            if result.returncode:
                failures += 1
                detail = result.stderr.decode("utf-8", errors="replace").strip()
                print(f"FAILED {source}: {detail or f'oj-bundle exited {result.returncode}'}", file=sys.stderr)
            else:
                bundled[source] = result.stdout
        if failures:
            print(f"Expansion failed: {failures}/{len(sources)} C++ sources; existing outputs and manifest preserved.", file=sys.stderr)
            return 1

        next_entries = dict(entries) if not args.clean else {}
        for source, content in bundled.items():
            target_name = output_name(source)
            write_atomic(ROOT / target_name, content)
            next_entries[source] = {"output": target_name, "sha256": digest(content)}

        removed = 0
        retained = 0
        if args.clean:
            for source, record in entries.items():
                if source in bundled:
                    continue
                path = ROOT / record["output"]
                check_path(path, expect_file=True)
                if path.exists():
                    if digest(path.read_bytes()) == record["sha256"]:
                        path.unlink()
                        removed += 1
                    else:
                        retained += 1
                        print(f"Retained modified orphan: {path}", file=sys.stderr)
        manifest_data = {"version": 1, "outputs": dict(sorted(next_entries.items()))}
        write_atomic(MANIFEST, (json.dumps(manifest_data, indent=2, ensure_ascii=False) + "\n").encode())
        print(f"Expanded {len(sources)} C++ source(s); removed {removed} owned orphan(s); retained {retained} modified orphan(s).")
        print(f"Python sources: {python_count} standalone file(s); Python expansion is not supported.")
        return 0
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"Expansion aborted: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
