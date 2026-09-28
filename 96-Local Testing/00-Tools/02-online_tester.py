#!/usr/bin/env python3
"""Fixture checks for online expansion and explicit workspace generation."""

from __future__ import annotations

import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
ONLINE = ROOT / "97-Online Testing"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def run(script: Path, *args: str, cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run([sys.executable, str(script), *args], cwd=cwd,
                          capture_output=True, text=True, timeout=20)


def test_expander(temp: Path) -> None:
    online = temp / "library with spaces" / "97-Online Testing"
    online.mkdir(parents=True)
    script = online / "01-expander.py"
    shutil.copy2(ONLINE / script.name, script)
    source_dir = online / "src" / "judge with spaces" / "topic"
    source_dir.mkdir(parents=True)
    a, b = source_dir / "a.cpp", source_dir / "b.cpp"
    a.write_text("A1\n", encoding="utf-8")
    b.write_text("B1\n", encoding="utf-8")
    (source_dir / "already.expanded.cpp").write_text("FAIL must not be input\n", encoding="utf-8")
    (source_dir / "native.py").write_text("print(1)\n", encoding="utf-8")
    bundler = temp / "fake bundler"
    bundler.write_text(
        "#!/usr/bin/env python3\n"
        "import sys, time\nfrom pathlib import Path\n"
        "source = Path(sys.argv[1]).read_text()\n"
        "if 'SLEEP' in source: time.sleep(2)\n"
        "if 'FAIL' in source:\n"
        "    print('forced failure', file=sys.stderr)\n"
        "    sys.exit(1)\n"
        "sys.stdout.write('bundled:' + source)\n", encoding="utf-8")
    bundler.chmod(0o755)

    def expand(*flags: str) -> subprocess.CompletedProcess[str]:
        return run(script, "--bundler", str(bundler), *flags, cwd=temp)

    output_a = online / "expanded" / "judge with spaces" / "topic" / "a.expanded.cpp"
    output_b = output_a.with_name("b.expanded.cpp")
    manifest = online / "02-expansion.json"
    result = expand()
    require(result.returncode == 0, f"initial expansion: {result.stderr}")
    require(output_a.read_text() == "bundled:A1\n" and output_b.read_text() == "bundled:B1\n",
            "recursive paths or bundled content differ")
    require(a.read_text() == "A1\n" and b.read_text() == "B1\n", "canonical sources changed")
    require("Python sources: 1" in result.stdout, "Python source was silently skipped")

    require(len(json.loads(manifest.read_text())["outputs"]) == 2, "expanded input was consumed")
    previous_manifest = manifest.read_bytes()
    a.write_text("A2\n", encoding="utf-8")
    b.write_text("FAIL\n", encoding="utf-8")
    result = expand()
    require(result.returncode != 0 and "forced failure" in result.stderr, "bundler failure not reported")
    require(output_a.read_text() == "bundled:A1\n" and output_b.read_text() == "bundled:B1\n",
            "failed batch changed existing outputs")
    require(manifest.read_bytes() == previous_manifest, "failed batch changed manifest")
    b.write_text("SLEEP\n", encoding="utf-8")
    result = expand("--timeout", "0.05")
    require(result.returncode != 0 and "exceeded" in result.stderr, "timeout not reported")
    require(manifest.read_bytes() == previous_manifest and output_a.read_text() == "bundled:A1\n", "timeout changed prior outputs")
    print("PASS online: recursive expansion, expanded-input exclusion, preservation and timeout")

    b.write_text("B2\n", encoding="utf-8")
    require(expand().returncode == 0 and output_a.read_text() == "bundled:A2\n",
            "successful expansion did not replace output")
    b.unlink()
    require(expand("--noclean").returncode == 0 and output_b.exists(), "--noclean removed orphan")
    require(expand("--clean").returncode == 0 and not output_b.exists(), "--clean left owned orphan")
    require(len(json.loads(manifest.read_text())["outputs"]) == 1, "orphan remains in manifest")
    a.unlink()
    output_a.write_text("user edit\n", encoding="utf-8")
    result = expand()
    require(result.returncode == 0 and output_a.read_text() == "user edit\n",
            "modified orphan was deleted")
    require("Retained modified orphan" in result.stderr, "modified orphan was not reported")
    require(not json.loads(manifest.read_text())["outputs"], "modified orphan still claimed")
    a.write_text("A3\n", encoding="utf-8")
    require(expand().returncode != 0 and output_a.read_text() == "user edit\n",
            "unowned output was overwritten")
    print("PASS online: clean/noclean and manifest ownership")

    output_a.unlink()
    outside = temp / "outside"
    outside.write_text("keep\n", encoding="utf-8")
    output_a.symlink_to(outside)
    require(expand().returncode != 0 and outside.read_text() == "keep\n",
            "output symlink was followed")
    output_a.unlink()
    manifest.write_text(json.dumps({"version": 1, "outputs": {
        "src/../outside.cpp": {"output": "../../outside", "sha256": "0" * 64}}}), encoding="utf-8")
    require(expand().returncode != 0 and outside.read_text() == "keep\n",
            "unsafe manifest entry was accepted")
    manifest.unlink()
    manifest.symlink_to(outside)
    require(expand().returncode != 0 and outside.read_text() == "keep\n",
            "manifest symlink was followed")
    print("PASS online: symlinks and unsafe manifest paths")


def test_workspace(temp: Path) -> None:
    library = temp / "workspace fixture with spaces"
    core = library / "01-Core"
    online = library / "97-Online Testing"
    core.mkdir(parents=True)
    online.mkdir()
    for name in ("01-template.hpp", "02-debug.hpp"):
        shutil.copy2(ROOT / "01-Core" / name, core / name)
    script = online / "03-workspace.py"
    shutil.copy2(ONLINE / script.name, script)
    target = library / "99-Workspace" / "template.cpp"
    require(run(script, "--check", cwd=temp).returncode != 0, "check accepted a missing snapshot")
    result = run(script, cwd=temp)
    require(result.returncode == 0, f"workspace generation: {result.stderr}")
    contents = target.read_text(encoding="utf-8")
    require("#pragma once" not in contents and '#include "' not in contents,
            "snapshot contains duplicate local directives")
    require(contents.count("void solve()") == 1 and contents.count("int main()") == 1,
            "snapshot has duplicate solve/main")
    require("std::ios::sync_with_stdio(false);" in contents and "cin.tie(nullptr);" in contents
            and "// while (t--) { solve(); }" in contents, "workspace main is incomplete")
    require(run(script, "--check", cwd=temp).returncode == 0, "fresh snapshot failed --check")
    with (core / "01-template.hpp").open("a", encoding="utf-8") as file:
        file.write("\n// fixture change\n")
    result = run(script, "--check", cwd=temp)
    require(result.returncode != 0 and "fixture change" in result.stdout,
            "stale snapshot did not produce a diff")
    require(target.read_text(encoding="utf-8") == contents, "--check modified the snapshot")
    require(run(script, cwd=temp).returncode == 0 and "fixture change" in target.read_text(),
            "explicit regeneration did not update snapshot")
    print("PASS online: script-relative workspace generation and check")


def main() -> int:
    try:
        with tempfile.TemporaryDirectory(prefix="online-tools-test-") as directory:
            temp = Path(directory)
            test_expander(temp)
            test_workspace(temp)
        return 0
    except (AssertionError, OSError, subprocess.TimeoutExpired) as exc:
        print(f"FAIL online tools: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
