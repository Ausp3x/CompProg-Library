#!/usr/bin/env python3
"""Independent CLI/protocol fixtures for 09-Contest Testing (Python 3.10+).

quick: batch/replay, judging modes, shrinking and quick-wrapper smoke regressions.
full: every named deterministic test, including CLI/build/timeout/cleanup protocols.
stress: full plus seeded token/byte comparisons and shrink proposal histories.
Modes/seeds accept --mode/--seed or the library runner's CP_TEST_MODE/CP_TEST_SEED.

Feature map: 01-stress -> batch*, checker*, mismatch*, scored*, interactive*,
configuration*, process*, cpp*,
input_validator_and_expected*; 02/03-gen -> generator_templates; 04-checker ->
checker_template; 05-interactor -> interactive*; 06/07 -> scored_templates;
08-shrink -> shrink*; 09-quick -> quick*, configuration*, process*.
Unittest checks remain active with python -O. Failed fixtures remain in /tmp.
"""
import argparse
import base64
from decimal import Decimal
import json
import os
import random
import signal
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "09-Contest Testing"
STRESS = TOOLS / "01-stress.py"
SHRINK = TOOLS / "08-shrink.py"
QUICK = TOOLS / "09-quick.py"
MODE, SEED = "full", 42
QUICK_TESTS = {
    "test_batch_pass_exact_and_cpp", "test_mismatch_replay_and_artifact_metadata",
    "test_batch_error_classes", "test_checker_without_reference",
    "test_scored_valid_gap_and_errors", "test_interactive_transcript_rejection_and_idle",
    "test_shrink_validity_and_signature", "test_quick_batch_entry",
}


class ContestTools(unittest.TestCase):
    def setUp(self):
        self.dir = Path(tempfile.mkdtemp(prefix="contest-tool-test-"))
        self.addCleanup(self.clean_fixture)
        self.last_command = None
        self.cwd = self.dir / "unrelated cwd"
        self.cwd.mkdir()
        self.artifacts = self.dir / "artifacts"
        self.gen = self.file("gen space.py", "import sys\nprint(sys.argv[1])\n")
        self.good = self.file("good space.py", "import sys\nprint(sys.stdin.read().strip())\n")
        self.bad = self.file("bad.py", "print(999)\n")

    def file(self, name, body):
        path = self.dir / name
        path.write_text(body)
        return path

    def clean_fixture(self):
        result = self._outcome.result
        if any(test is self or getattr(test, "test_case", None) is self
               for test, _ in result.failures + result.errors):
            print(f"\nREPRO seed={SEED} mode={MODE} test={self.id()}\n"
                  f"fixture={self.dir} cwd={self.cwd} command={self.last_command}",
                  file=sys.stderr)
        else:
            shutil.rmtree(self.dir)

    def run_raw(self, command, *, code=0, data=None, timeout=20):
        self.last_command = list(map(str, command))
        result = subprocess.run(self.last_command, input=data, cwd=self.cwd,
                                capture_output=True, text=True, timeout=timeout)
        self.assertEqual(result.returncode, code,
                         f"seed={SEED} cwd={self.cwd} command={self.last_command}\n"
                         f"stdout: {result.stdout}\nstderr: {result.stderr}")
        return result

    def run_tool(self, tool, *args, code=0):
        command = [sys.executable, str(tool), *map(str, args)]
        if tool != SHRINK:
            command += ["--artifacts", str(self.artifacts)]
        return self.run_raw(command, code=code)

    def failure(self, kind):
        folder = self.latest_failure()
        meta = json.loads((folder / "case.json").read_text())
        self.assertEqual(meta["kind"], kind)
        return folder, meta

    def latest_failure(self):
        return max(self.artifacts.iterdir(), key=lambda path: path.stat().st_mtime_ns)

    def test_batch_pass_exact_and_cpp(self):
        result = self.run_tool(STRESS, self.gen, self.good, self.good, "--count", 2)
        self.assertIn("seed=2 pass", result.stdout)
        self.run_tool(STRESS, self.gen, self.good, self.good, "--count", 1, "--exact")
        if shutil.which("g++") is None:
            self.skipTest("g++ unavailable for mixed C++/Python fixture")
        cpp = self.file("candidate space.cpp", "#include <bits/stdc++.h>\nusing namespace std;\n"
                        "using lng = int64_t;\n\n"
                        "int main() {\n    lng x; cin >> x;\n    cout << x << '\\n';}\n")
        self.assertIn("pass", self.run_tool(STRESS, self.gen, cpp, self.good, "--count", 1).stdout)

    def test_mismatch_replay_and_artifact_metadata(self):
        result = self.run_tool(STRESS, self.gen, self.bad, self.good,
                               "--seed", 42, "--count", 1, code=1)
        self.assertIn("wrong_answer", result.stdout)
        folder = self.latest_failure()
        self.assertEqual((folder / "input.txt").read_bytes(), b"42\n")
        self.assertEqual((folder / "candidate.out").read_bytes(), b"999\n")
        meta = json.loads((folder / "case.json").read_text())
        self.assertEqual(meta["kind"], "wrong_answer")
        self.assertEqual(meta["config"]["comparison"], "tokens")
        self.assertEqual(meta["config"]["cwd"], str(self.cwd))
        self.assertEqual(meta["sources"]["candidate"], str(self.bad))
        self.assertIn("wrong_answer", self.run_tool(STRESS, self.gen, self.bad, self.good,
                            "--replay", folder / "input.txt", code=1).stdout)

    def test_batch_error_classes(self):
        crash = self.file("crash.py", "raise RuntimeError('boom')\n")
        slow = self.file("slow.py", "import time\ntime.sleep(2)\n")
        generator_error = self.file("generator_error.py", "import sys\nsys.exit(5)\n")
        checker_error = self.file("checker_error.py", "import sys\nsys.exit(2)\n")
        cases = [
            ((self.gen, crash, self.good), "candidate_crash"),
            ((self.gen, slow, self.good, "--timeout", 0.1), "candidate_timeout"),
            ((generator_error, self.good, self.good), "generator_error"),
            ((self.gen, self.good, crash), "reference_error"),
            ((self.gen, self.good, self.good, "--checker", checker_error), "checker_error"),
        ]
        for args, kind in cases:
            with self.subTest(kind=kind):
                self.assertIn(kind, self.run_tool(STRESS, *args, "--count", 1, code=1).stdout)
                self.assertEqual(json.loads((self.latest_failure() / "case.json").read_text())["kind"], kind)

    def test_checker_without_reference(self):
        checker = self.file("checker.py", "import sys\nfrom pathlib import Path\nsys.exit(0 if Path(sys.argv[3]).read_bytes()==b'' else 2)\n")
        self.assertIn("pass", self.run_tool(STRESS, self.gen, self.good,
                      "--checker", checker, "--count", 1).stdout)

    def test_scored_valid_gap_and_errors(self):
        gen = self.file("score_gen.py", "print(3)\n")
        candidate = self.file("score_candidate.py", "print('3 2 1')\n")
        reference = self.file("score_reference.py", "print('1 2 3')\n")
        options = ("--mode", "scored", "--validator", TOOLS / "06-validator.py",
                   "--scorer", TOOLS / "07-scorer.py", "--count", 1)
        result = self.run_tool(STRESS, gen, candidate, reference, *options)
        self.assertIn("candidate=0", result.stdout)
        self.assertIn("gap=2", result.stdout)
        self.assertIn("candidate=0", self.run_tool(STRESS, gen, candidate, *options).stdout)
        invalid = self.file("invalid.py", "print('1 1 1')\n")
        self.assertIn("invalid_output", self.run_tool(STRESS, gen, invalid, *options, code=1).stdout)
        nan = self.file("nan.py", "print('nan')\n")
        bad_options = ("--mode", "scored", "--validator", TOOLS / "06-validator.py",
                       "--scorer", nan, "--count", 1)
        self.assertIn("scorer_error", self.run_tool(STRESS, gen, candidate, *bad_options, code=1).stdout)

    def test_interactive_transcript_rejection_and_idle(self):
        hidden = self.file("hidden.py", "print(7)\n")
        candidate = self.file("candidate.py", "import sys\nsys.exit(2) if input()!='guess' else None\nprint(7,flush=True)\n")
        args = ("--mode", "interactive", "--interactor", TOOLS / "05-interactor.py", "--count", 1)
        self.assertIn("pass", self.run_tool(STRESS, hidden, candidate, *args).stdout)
        wrong = self.file("wrong.py", "import sys\nsys.exit(2) if input()!='guess' else None\nprint(8,flush=True)\n")
        self.assertIn("interactor_rejected", self.run_tool(STRESS, hidden, wrong, *args, code=1).stdout)
        folder = self.latest_failure()
        self.assertEqual((folder / "candidate_to_interactor.bin").read_bytes(), b"8\n")
        self.assertEqual((folder / "interactor_to_candidate.bin").read_bytes(), b"guess\n")
        self.assertTrue((folder / "transcript.jsonl").read_text())
        wait = self.file("wait.py", "import sys,time\nsys.exit(2) if input()!='guess' else None\ntime.sleep(3)\n")
        result = self.run_tool(STRESS, hidden, wait, *args, "--idle-timeout", 0.2,
                               "--whole-timeout", 1, code=1)
        self.assertIn("interactive_idle_timeout", result.stdout)

    def test_shrink_validity_and_signature(self):
        original = self.file("large.in", "10 20 30\n")
        proposer = self.file("propose.py", "import sys\nprint('invalid' if sys.argv[2]=='0' else '10 20' if sys.argv[2]=='1' else '10')\n")
        valid = self.file("valid.py", "import sys\ns=open(sys.argv[1]).read()\nsys.exit(0 if s.startswith('10') else 1)\n")
        probe = self.file("probe.py", "import sys\ns=open(sys.argv[1]).read()\nprint('wrong_answer:A' if '20' in s else 'other')\n")
        reduced = self.dir / "reduced.in"
        result = self.run_tool(SHRINK, original, "--propose", proposer, "--valid", valid,
                               "--probe", probe, "--max-steps", 3, "--output", reduced)
        self.assertEqual(reduced.read_bytes(), b"10 20\n")
        self.assertEqual(original.read_bytes(), b"10 20 30\n")
        self.assertIn("1 accepted", result.stdout)

    def test_quick_batch_entry(self):
        self.assertIn("pass", self.run_tool(QUICK, self.gen, self.good, self.good, "--count", 1).stdout)
        self.assertIn("wrong_answer", self.run_tool(QUICK, self.gen, self.bad, self.good, "--count", 1, code=1).stdout)
        self.assertTrue((self.latest_failure() / "input.txt").is_file())

    def test_batch_tokens_bytes_large_integers_and_stderr(self):
        spaced = self.file("spaced.py", "import sys\nsys.stdout.write(' 1\\t2 \\n')\nprint('diagnostic',file=sys.stderr)\n")
        plain = self.file("plain.py", "print('1 2')\n")
        self.run_tool(STRESS, self.gen, spaced, plain, "--count", 1)
        self.run_tool(STRESS, self.gen, spaced, plain, "--count", 1, "--exact", code=1)
        folder, meta = self.failure("wrong_answer")
        self.assertEqual(meta["config"]["comparison"], "bytes")
        self.assertEqual((folder / "candidate.err").read_bytes(), b"diagnostic\n")
        self.assertEqual((folder / "candidate.out").read_bytes(), b" 1\t2 \n")
        left = self.file("large_a.py", "print(9007199254740992)\n")
        right = self.file("large_b.py", "print(9007199254740993)\n")
        self.run_tool(STRESS, self.gen, left, right, "--count", 1, code=1)
        self.failure("wrong_answer")
        empty = self.file("empty.py", "pass\n")
        self.run_tool(STRESS, empty, empty, empty, "--count", 1)

    def test_mismatch_replay_ignores_generator_and_unique_artifacts(self):
        for _ in range(2):
            self.run_tool(STRESS, self.gen, self.bad, self.good, "--count", 1, "--seed", -17, code=1)
        folders = list(self.artifacts.iterdir())
        self.assertEqual(len(folders), 2)
        folder, meta = self.failure("wrong_answer")
        self.assertEqual(meta["seed"], -17)
        self.assertEqual(meta["commands"]["generator"][-1], "-17")
        self.assertEqual(meta["statuses"]["candidate"], "ok")
        before = {x: (x / "input.txt").read_bytes() for x in folders}
        # A missing generator proves replay reads the saved case, not the seed.
        result = self.run_tool(STRESS, self.dir / "missing.py", self.bad, self.good,
                               "--replay", folder / "input.txt", "--count", 3, code=1)
        self.assertEqual(result.stdout.count("seed="), 1)
        self.assertEqual(len(list(self.artifacts.iterdir())), 3)
        for path, data in before.items():
            self.assertEqual((path / "input.txt").read_bytes(), data)
        self.assertNotIn("generator", self.failure("wrong_answer")[1]["sources"])
        self.run_tool(STRESS, self.gen, self.good, self.good,
                      "--replay", self.dir / "absent.in", code=2)

    def test_checker_status_paths_and_reference_failure_order(self):
        checker = self.file("path checker.py", "import sys\nfrom pathlib import Path\n"
                            "a,b,c=map(Path,sys.argv[1:])\n"
                            "sys.exit(0 if a.read_bytes()==b'1\\n' and b.read_bytes()==b'999\\n' and c.read_bytes()==b'1\\n' else 2)\n")
        self.run_tool(STRESS, self.gen, self.bad, self.good, "--checker", checker, "--count", 1)
        for body, expected in (("raise SystemExit(1)\n", "wrong_answer"),
                               ("raise SystemExit(2)\n", "checker_error"),
                               ("import time\ntime.sleep(3)\n", "checker_error")):
            checker.write_text(body)
            self.run_tool(STRESS, self.gen, self.good, self.good, "--checker", checker,
                          "--timeout", 0.2, "--count", 1, code=1)
            _, meta = self.failure(expected)
            self.assertEqual([Path(path).name for path in meta["commands"]["checker"][-3:]],
                             ["input.txt", "candidate.txt", "reference.txt"])
        marker = self.dir / "candidate_ran"
        candidate = self.file("must_not_run.py", f"from pathlib import Path\nPath({str(marker)!r}).touch()\n")
        crash = self.file("reference_crash.py", "raise SystemExit(2)\n")
        self.run_tool(STRESS, self.gen, candidate, crash, "--checker", checker, "--count", 1, code=1)
        self.failure("reference_error")
        self.assertFalse(marker.exists(), "a failed trusted reference must make judging inconclusive")

    def test_configuration_rejects_invalid_limits_and_missing_programs(self):
        for tool in (STRESS, QUICK):
            for option, value in (("--count", "-1"), ("--timeout", "0"),
                                  ("--timeout", "nan"), ("--timeout", "inf")):
                with self.subTest(tool=tool.name, option=option, value=value):
                    self.run_tool(tool, self.gen, self.good, self.good, option, value, code=2)
            self.run_tool(tool, self.gen, self.dir / "missing.py", self.good, "--count", 1, code=2)
            plain = self.file("not executable", "contents\n")
            self.run_tool(tool, self.gen, plain, self.good, "--count", 1, code=2)
        for option in ("--whole-timeout", "--idle-timeout"):
            for value in ("-1", "nan", "inf"):
                self.run_tool(STRESS, self.gen, self.good, self.good, option, value, code=2)
        self.run_tool(STRESS, self.gen, self.good, code=2)
        self.run_tool(STRESS, self.gen, self.good, "--mode", "scored", code=2)
        self.run_tool(STRESS, self.gen, self.good, "--mode", "interactive", code=2)
        self.run_tool(STRESS, self.gen, self.good, self.good, "--mode", "interactive",
                      "--interactor", TOOLS / "05-interactor.py", code=2)
        self.run_tool(STRESS, self.gen, self.good, self.good,
                      "--interactor", TOOLS / "05-interactor.py", code=2)
        self.run_tool(STRESS, self.gen, self.good, self.good,
                      "--validator", TOOLS / "06-validator.py", code=2)
        self.run_tool(STRESS, self.gen, self.good, "--mode", "scored",
                      "--validator", TOOLS / "06-validator.py", "--scorer", TOOLS / "07-scorer.py",
                      "--checker", TOOLS / "04-checker.py", code=2)
        self.run_tool(STRESS, self.gen, self.good, self.good, "--exact",
                      "--checker", TOOLS / "04-checker.py", code=2)

    def test_input_validator_and_expected_answer_replay(self):
        validator = self.file("input validator.py", "import sys\nfrom pathlib import Path\n"
                              "x = int(Path(sys.argv[1]).read_bytes())\n"
                              "sys.exit(2 if x == 7 else 0 if x % 3 else 1)\n")
        self.run_tool(STRESS, self.gen, self.bad, self.good, "--count", 2, "--input-validator", validator, code=1)
        folder, meta = self.failure("wrong_answer")
        self.assertEqual(meta["statuses"]["input_validator"], "ok")
        self.assertTrue((folder / "input_validator.err").exists())
        self.run_tool(STRESS, self.gen, self.good, self.good, "--count", 2, "--input-validator", validator)
        self.run_tool(STRESS, "unused", self.good, self.good, "--replay", self.file("valid.in", "5\n"),
                      "--input-validator", validator)
        broken = self.file("broken gen.py", "import sys\nsys.exit(3)\n")
        self.run_tool(STRESS, broken, self.good, self.good, "--count", 1, "--input-validator", validator, code=1)
        _, meta = self.failure("generator_error")
        self.assertNotIn("input_validator", meta["statuses"])
        self.assertEqual(meta["commands"]["input_validator"], [sys.executable, str(validator)])
        scored = ("--mode", "scored", "--validator", TOOLS / "06-validator.py", "--scorer", TOOLS / "07-scorer.py")
        for mode in ((), scored):
            for seed, kind in ((3, "invalid_input"), (7, "input_validator_error")):
                with self.subTest(mode=mode, seed=seed):
                    self.run_tool(STRESS, self.gen, self.good, self.good, *mode, "--seed", seed, "--count", 1,
                                  "--input-validator", validator, code=1)
                    folder, meta = self.failure(kind)
                    self.assertEqual(set(meta["statuses"]), {"generator", "input_validator"})
                    self.assertEqual(Path(meta["commands"]["input_validator"][-1]).name, "input.txt")
                    self.assertFalse((folder / "candidate.out").exists())
        hidden = self.file("hidden.in", "6\n")
        self.run_tool(STRESS, "unused", self.good, "--mode", "interactive", "--interactor",
                      TOOLS / "05-interactor.py", "--replay", hidden, "--input-validator", validator, code=1)
        folder, meta = self.failure("invalid_input")
        self.assertEqual(set(meta["statuses"]), {"input_validator"})
        self.assertFalse((folder / "transcript.jsonl").exists())
        saved, answer = self.file("saved.in", "5\n"), self.file("saved.ans", "5\n")
        self.run_tool(STRESS, "unused", self.good, "--replay", saved, "--expected", answer)
        self.run_tool(STRESS, "unused", self.good, "--replay", saved, "--expected", answer, "--exact")
        self.run_tool(STRESS, "unused", self.good, "--replay", saved, "--expected", answer,
                      "--checker", TOOLS / "04-checker.py")
        answer.write_text("6\n")
        self.run_tool(STRESS, "unused", self.good, "--replay", saved, "--expected", answer, code=1)
        folder, meta = self.failure("wrong_answer")
        self.assertEqual((folder / "reference.out").read_bytes(), b"6\n")
        self.assertEqual(meta["config"]["expected"], str(answer))
        for args in ((self.gen, self.good, "--expected", answer),
                     ("unused", self.good, self.good, "--replay", saved, "--expected", answer),
                     ("unused", self.good, *scored, "--replay", saved, "--expected", answer),
                     ("unused", self.good, "--replay", saved, "--expected", self.dir / "missing.ans")):
            with self.subTest(args=args):
                self.run_tool(STRESS, *args, code=2)

    def test_cpp_compile_once_flags_executables_and_failures(self):
        compiler = shutil.which("g++")
        if compiler is None:
            self.skipTest("g++ unavailable: C++ compilation and generator fixture")
        log = self.dir / "compilations.txt"
        wrapper = self.file("compiler wrapper", f"#!{sys.executable}\nimport subprocess,sys\n"
                            f"with open({str(log)!r},'a') as f: f.write('compile\\n')\n"
                            f"sys.exit(subprocess.call([{compiler!r}]+sys.argv[1:]))\n")
        wrapper.chmod(0o700)
        cpp = self.file("candidate space.cpp", "#include <cstdint>\n#include <iostream>\n"
                        "#ifndef FIXTURE\n#error missing flag\n#endif\n"
                        "using lng = int64_t;\n\n"
                        "int main() {\n    lng x; std::cin >> x;\n    std::cout << x << '\\n';}\n")
        self.run_tool(STRESS, self.gen, cpp, self.good, "--count", 3, "--python", sys.executable,
                      "--cxx", wrapper, "--cxxflags=-std=gnu++20 -O2 -DFIXTURE")
        self.assertEqual(log.read_text(), "compile\n")
        self.assertEqual(list(self.dir.glob("*.o")), [])
        executable = self.file("existing executable", f"#!{sys.executable}\nimport sys\nprint(sys.stdin.read().strip())\n")
        executable.chmod(0o700)
        self.run_tool(STRESS, self.gen, executable, self.good, "--count", 1)
        self.run_tool(QUICK, self.gen, executable, self.good, "--count", 1)
        cpp.write_text("#include <iostream>\n\nint main() { std::cout << std::cin.rdbuf(); }\n")
        self.run_tool(QUICK, self.gen, cpp, self.good, "--count", 2)
        cpp.write_text("this does not compile\n")
        for tool in (STRESS, QUICK):
            result = self.run_tool(tool, self.gen, cpp, self.good, "--count", 1, code=2)
            self.assertIn("compile failed", result.stderr)
            self.assertNotIn("Traceback", result.stderr)

    def test_batch_and_quick_role_timeouts_and_launch_errors(self):
        slow = self.file("slow.py", "import sys,time\nprint('partial',flush=True)\nprint('diagnostic',file=sys.stderr,flush=True)\ntime.sleep(3)\n")
        for tool in (STRESS, QUICK):
            for index, kind, role in ((0, "generator_error", "generator"),
                                      (1, "candidate_timeout", "candidate"),
                                      (2, "reference_error", "reference")):
                programs = [self.gen, self.good, self.good]
                programs[index] = slow
                with self.subTest(tool=tool.name, role=role):
                    self.run_tool(tool, *programs, "--count", 1, "--timeout", 0.2, code=1)
                    folder, meta = self.failure(kind)
                    self.assertEqual(meta["statuses"][role], "timeout")
                    self.assertEqual((folder / f"{role}.out").read_bytes(), b"partial\n")
                    self.assertEqual((folder / f"{role}.err").read_bytes(), b"diagnostic\n")
        invalid = self.file("bad executable", "#!/does/not/exist\n")
        invalid.chmod(0o700)
        self.run_tool(STRESS, self.gen, invalid, self.good, "--count", 1, code=1)
        self.assertIn("launch error", self.failure("candidate_crash")[1]["statuses"]["candidate"])

    def test_scored_objectives_precision_and_finite_output(self):
        valid = self.file("accept.py", "pass\n")
        scorer = self.file("read_score.py", "import sys\nfrom pathlib import Path\nprint(Path(sys.argv[2]).read_text())\n")
        options = ("--mode", "scored", "--validator", valid, "--scorer", scorer, "--count", 1)
        for candidate, reference, expected in (("7.5", "10", "2.5"),
                                                ("10000000000000000000000000000000000000000",
                                                 "10000000000000000000000000000000000000001", "1"),
                                                ("0", "1e1000000", "1e1000000")):
            left = self.file("candidate_score.py", f"print({candidate!r})\n")
            right = self.file("reference_score.py", f"print({reference!r})\n")
            for objective, sign in (("maximize", ""), ("minimize", "-")):
                result = self.run_tool(STRESS, self.gen, left, right, *options, "--objective", objective)
                gap = next(x[4:] for x in result.stdout.split() if x.startswith("gap="))
                self.assertEqual(Decimal(gap), Decimal(sign + expected))
        # A representable score may have a gap too small for Decimal arithmetic;
        # reporting that nonzero gap as zero would falsely claim equal quality.
        left.write_text("print('1e-1000000000000000100')\n")
        right.write_text("print(0)\n")
        for objective in ("maximize", "minimize"):
            result = self.run_tool(STRESS, self.gen, left, right, *options,
                                   "--objective", objective, code=1)
            self.failure("scorer_error")
            self.assertNotIn("gap=", result.stdout)
        for raw in ("nan", "sNaN", "inf", "-Infinity", "", "1 2", "no-score"):
            with self.subTest(score=raw):
                self.bad.write_text(f"print({raw!r})\n")
                self.run_tool(STRESS, self.gen, self.bad, *options, code=1)
                self.failure("scorer_error")

    def test_scored_validator_and_scorer_statuses(self):
        valid = self.file("accept.py", "pass\n")
        scorer = self.file("score.py", "print(1)\n")
        options = ("--mode", "scored", "--validator", valid, "--scorer", scorer,
                   "--count", 1, "--timeout", 0.2)
        for hook, body, kind in ((valid, "raise SystemExit(1)\n", "invalid_output"),
                                 (valid, "raise SystemExit(2)\n", "validator_error"),
                                 (valid, "import time\ntime.sleep(3)\n", "validator_error"),
                                 (scorer, "raise SystemExit(1)\n", "scorer_error"),
                                 (scorer, "import time\ntime.sleep(3)\n", "scorer_error")):
            valid.write_text("pass\n")
            scorer.write_text("print(1)\n")
            hook.write_text(body)
            self.run_tool(STRESS, self.gen, self.good, *options, code=1)
            self.failure(kind)
        valid.write_text("raise SystemExit(1)\n")
        self.run_tool(STRESS, self.gen, self.good, self.good, *options, code=1)
        self.failure("reference_error")

    def test_interactive_protocol_error_classes_and_total_timeout(self):
        hidden = self.file("hidden.py", "print(7)\n")
        interactor = self.file("interaction.py", "raise SystemExit(2)\n")
        empty = self.file("empty.py", "pass\n")
        options = ("--mode", "interactive", "--interactor", interactor, "--count", 1)
        self.run_tool(STRESS, hidden, empty, *options, code=1)
        self.failure("interactor_error")
        interactor.write_text("pass\n")
        self.run_tool(STRESS, hidden, self.file("crash.py", "raise SystemExit(4)\n"), *options, code=1)
        self.failure("candidate_crash")
        ping = "import sys\nprint('ping',flush=True)\nfor line in sys.stdin: print(line.strip(),flush=True)\n"
        interactor.write_text(ping)
        candidate = self.file("ping.py", ping)
        self.run_tool(STRESS, hidden, candidate, *options, "--whole-timeout", 0.2,
                      "--idle-timeout", 1, code=1)
        folder, _ = self.failure("interactive_whole_timeout")
        rows = [json.loads(line) for line in (folder / "transcript.jsonl").read_text().splitlines()]
        self.assertGreater(len(rows), 0)
        for direction in ("candidate_to_interactor", "interactor_to_candidate"):
            reconstructed = b"".join(base64.b64decode(row["base64"]) for row in rows if row["direction"] == direction)
            self.assertEqual(reconstructed, (folder / f"{direction}.bin").read_bytes())
        self.assertEqual([row["seconds"] for row in rows], sorted(row["seconds"] for row in rows))

    def test_interactive_large_transfer_and_template_queries(self):
        hidden = self.file("hidden.py", "print(7)\n")
        candidate = self.file("guess.py", "import sys\n"
                              "if input()!='guess':raise SystemExit(2)\n"
                              "print('? 3',flush=True)\nif input()!='>':raise SystemExit(2)\n"
                              "print('? 10',flush=True)\nif input()!='<':raise SystemExit(2)\n"
                              "print('? 7',flush=True)\nif input()!='=':raise SystemExit(2)\nprint(7,flush=True)\n")
        options = ("--mode", "interactive", "--interactor", TOOLS / "05-interactor.py", "--count", 1)
        self.run_tool(STRESS, hidden, candidate, *options)
        payload = 256 * 1024
        interactor = self.file("large interactor.py", "import sys\n"
                               f"sys.stdout.write('x'*{payload}+'\\n');sys.stdout.flush()\n"
                               f"sys.exit(1 if sys.stdin.readline()=='y'*{payload}+'\\n' else 2)\n")
        candidate.write_text("import sys\n"
                             f"line=sys.stdin.readline()\nsys.exit(2) if line!='x'*{payload}+'\\n' else None\n"
                             f"sys.stdout.write('y'*{payload}+'\\n');sys.stdout.flush()\n")
        self.run_tool(STRESS, hidden, candidate, "--mode", "interactive", "--interactor", interactor,
                      "--count", 1, code=1)
        folder, _ = self.failure("interactor_rejected")
        self.assertEqual((folder / "candidate_to_interactor.bin").stat().st_size, payload + 1)
        self.assertEqual((folder / "interactor_to_candidate.bin").stat().st_size, payload + 1)

    def test_interactive_template_rejection_and_hidden_errors(self):
        hidden = self.file("hidden.in", "7\n")
        command = [sys.executable, TOOLS / "05-interactor.py", hidden]
        for text in ("", "bad\n", "? nope\n", "? 7\n" * 9, "8\n"):
            with self.subTest(protocol=text):
                self.run_raw(command, data=text, code=1)
        for text in ("bad\n", "101\n", "1 2\n"):
            hidden.write_text(text)
            self.run_raw(command, data="7\n", code=2)
        self.run_raw([sys.executable, TOOLS / "05-interactor.py"], code=2)

    def test_generator_templates_seeded_domain_and_cpp(self):
        commands = [[sys.executable, TOOLS / "02-gen.py"]]
        compiler = shutil.which("g++")
        if compiler:
            binary = self.dir / "generator executable"
            self.run_raw([compiler, "-std=gnu++20", "-O2", TOOLS / "03-gen.cpp", "-o", binary])
            commands.append([binary])
        for command in commands:
            for seed in (0, 1, 42, 2**64 - 1):
                with self.subTest(command=command, seed=seed):
                    first = self.run_raw([*command, seed]).stdout
                    self.assertEqual(first, self.run_raw([*command, seed]).stdout)
                    n, *values = map(int, first.split())
                    self.assertEqual(n, len(values))
                    self.assertTrue(1 <= n <= 20)
                    self.assertTrue(all(-100 <= value <= 100 for value in values))
            self.run_raw(command, code=2)
            self.run_raw([*command, "not-a-seed"], code=2)
        self.run_raw([*commands[0], "-1"])
        if compiler:
            for seed in ("-1", str(2**64), "12junk", "1.0"):
                self.run_raw([*commands[1], seed], code=2)
        if not compiler:
            print("SKIP: g++ unavailable for 03-gen.cpp fixture", file=sys.stderr)

    def test_checker_template_exact_integer_tokens_and_errors(self):
        data = self.file("input", "unused\n")
        actual = self.file("actual", " 9007199254740992\t2\n")
        reference = self.file("reference", "9007199254740992 2\n")
        command = [sys.executable, TOOLS / "04-checker.py", data, actual, reference]
        self.run_raw(command)
        reference.write_text("9007199254740993 2\n")
        self.run_raw(command, code=1)
        self.run_raw(command[:-1], code=2)

    def test_scored_templates_boundaries_and_error_contract(self):
        data = self.file("input", "3\n")
        output = self.file("output", "1 2 3\n")
        validator = [sys.executable, TOOLS / "06-validator.py", data, output]
        scorer = [sys.executable, TOOLS / "07-scorer.py", data, output]
        for text, score in (("1 2 3\n", 2), ("3 2 1\n", 0), ("2 1 3\n", 1)):
            output.write_text(text)
            self.run_raw(validator)
            self.assertEqual(int(self.run_raw(scorer).stdout), score)
        for text in ("", "1 1 1", "0 1 2", "1 2 3 4", "1 two 3"):
            output.write_text(text)
            self.run_raw(validator, code=1)
        data.write_text("0\n")
        output.write_text("")
        self.run_raw(validator)
        self.assertEqual(self.run_raw(scorer).stdout.strip(), "0")
        for text in ("", "-1", "bad"):
            data.write_text(text)
            self.run_raw(validator, code=2)
        self.run_raw(validator[:-1], code=2)
        self.run_raw(scorer[:-1], code=2)
        output.write_text("bad")
        self.run_raw(scorer, code=2)

    def shrink_hooks(self):
        original = self.file("original.in", "10 20 30\n")
        proposer = self.file("propose.py", "print('10 20')\n")
        valid = self.file("valid.py", "pass\n")
        probe = self.file("probe.py", "print('wrong_answer:A')\n")
        output = self.dir / "reduced.in"
        options = (original, "--propose", proposer, "--valid", valid, "--probe", probe,
                   "--output", output, "--max-steps", 6)
        return original, proposer, valid, probe, output, options

    def test_shrink_exact_signature_size_skip_and_attempt_bound(self):
        original, proposer, valid, probe, output, options = self.shrink_hooks()
        attempts = self.dir / "attempts.txt"
        proposer.write_text("import sys\n"
                            f"with open({str(attempts)!r},'a') as f:f.write(sys.argv[2]+'\\n')\n"
                            "step=int(sys.argv[2])\n"
                            "if step==4:raise SystemExit(3)\n"
                            "print(['bad','99 99 99','10','10 20','','20'][step])\n")
        valid.write_text("import sys\nfrom pathlib import Path\ns=Path(sys.argv[1]).read_text()\n"
                         "sys.exit(1 if s.startswith('bad') else 0)\n")
        probe.write_text("import sys\nfrom pathlib import Path\ns=Path(sys.argv[1]).read_text()\n"
                         "sys.stdout.write('wrong_answer:A\\n' if '20' in s else 'wrong_answer:A \\n')\n")
        result = self.run_tool(SHRINK, *options)
        self.assertEqual(output.read_bytes(), b"20\n")
        self.assertEqual(original.read_bytes(), b"10 20 30\n")
        self.assertEqual(attempts.read_text().splitlines(), list(map(str, range(6))))
        self.assertIn("2 accepted", result.stdout)
        # The accepted file is exclusive; neither it nor the original is overwritten.
        self.run_tool(SHRINK, *options, code=2)
        self.assertEqual(output.read_bytes(), b"20\n")
        self.assertEqual(attempts.read_text().splitlines(), list(map(str, range(6))))

    def test_shrink_initial_failures_and_invalid_limits(self):
        original, proposer, valid, probe, output, options = self.shrink_hooks()
        marker = self.dir / "probe_ran"
        probe.write_text(f"from pathlib import Path\nPath({str(marker)!r}).touch()\nprint('signature')\n")
        valid.write_text("raise SystemExit(1)\n")
        self.run_tool(SHRINK, *options, code=2)
        self.assertFalse(marker.exists(), "invalid baseline must not reach the failure probe")
        self.assertFalse(output.exists())
        valid.write_text("pass\n")
        probe.write_text("print('  ')\n")
        self.run_tool(SHRINK, *options, code=2)
        self.assertFalse(output.exists())
        for option, value in (("--timeout", "0"), ("--timeout", "nan"), ("--timeout", "inf"),
                              ("--max-steps", "0"), ("--max-steps", "-1")):
            self.run_tool(SHRINK, *options, option, value, code=2)
        self.run_tool(SHRINK, *options, "--propose", self.dir / "absent.py", code=2)

    def test_shrink_hook_failures_preserve_last_accepted_case(self):
        original, proposer, valid, probe, output, options = self.shrink_hooks()
        for role in ("propose", "valid", "probe"):
            with self.subTest(role=role):
                proposer.write_text("import sys\nprint('10 20' if sys.argv[2]=='0' else '10')\n")
                valid.write_text("pass\n")
                probe.write_text("print('wrong_answer:A')\n")
                hook = {"propose": proposer, "valid": valid, "probe": probe}[role]
                if role == "propose":
                    hook.write_text("import sys\nif sys.argv[2]!='0':raise SystemExit(2)\nprint('10 20')\n")
                else:
                    hook.write_text("import sys\nfrom pathlib import Path\n"
                                    "if Path(sys.argv[1]).read_bytes()==b'10\\n':raise SystemExit(2)\n"
                                    + ("print('wrong_answer:A')\n" if role == "probe" else ""))
                result = self.run_tool(SHRINK, *options, code=2)
                self.assertIn(role, result.stderr)
                self.assertEqual(output.read_bytes(), b"10 20\n")
                self.assertEqual(original.read_bytes(), b"10 20 30\n")
                output.unlink()

    def child_fixture(self, name):
        pids = self.dir / f"{name}.pids"
        program = self.file(f"{name}.py", "import os,subprocess,sys,time\nfrom pathlib import Path\n"
                            "child=subprocess.Popen([sys.executable,'-c','import time;time.sleep(60)'])\n"
                            f"Path({str(pids)!r}).write_text(str(os.getpid())+' '+str(child.pid))\n"
                            "time.sleep(60)\n")
        self.addCleanup(self.kill_fixture_processes, pids)
        return program, pids

    @staticmethod
    def process_live(pid):
        try:
            os.kill(pid, 0)
            status = Path(f"/proc/{pid}/stat")
            return not status.exists() or status.read_text().split(")", 1)[1].split()[0] != "Z"
        except (ProcessLookupError, FileNotFoundError):
            return False

    @staticmethod
    def kill_fixture_processes(path):
        for pid in map(int, path.read_text().split()) if path.exists() else ():
            try:
                os.kill(pid, signal.SIGKILL)
            except ProcessLookupError:
                pass

    def check_processes_stopped(self, path):
        self.assertTrue(path.exists(), "child fixture failed to reach the timeout/interruption point")
        pids = list(map(int, path.read_text().split()))
        deadline = time.monotonic() + 2
        while any(self.process_live(pid) for pid in pids) and time.monotonic() < deadline:
            time.sleep(0.01)
        self.assertFalse(any(self.process_live(pid) for pid in pids), f"leaked child PIDs: {pids}")

    @unittest.skipUnless(os.name == "posix", "process-group fixtures need POSIX")
    def test_process_timeout_cleans_descendants_in_all_drivers(self):
        for tool in (STRESS, QUICK):
            candidate, pids = self.child_fixture(tool.stem)
            self.run_tool(tool, self.gen, candidate, self.good, "--count", 1, "--timeout", 0.3, code=1)
            self.failure("candidate_timeout")
            self.check_processes_stopped(pids)
        candidate, pids = self.child_fixture("interactive_child")
        interactor = self.file("wait_interactor.py", "import time\ntime.sleep(60)\n")
        self.run_tool(STRESS, self.gen, candidate, "--mode", "interactive", "--interactor", interactor,
                      "--count", 1, "--whole-timeout", 0.3, "--idle-timeout", 1, code=1)
        self.failure("interactive_whole_timeout")
        self.check_processes_stopped(pids)
        # Both leaders may exit successfully while a descendant still owns stdout.
        # The relay must wait for EOF and time out, rather than falsely accepting.
        candidate, pids = self.child_fixture("inherited_pipe")
        candidate.write_text(candidate.read_text().removesuffix("time.sleep(60)\n"))
        interactor.write_text("pass\n")
        self.run_tool(STRESS, self.gen, candidate, "--mode", "interactive", "--interactor", interactor,
                      "--count", 1, "--whole-timeout", 0.3, "--idle-timeout", 1, code=1)
        _, meta = self.failure("interactive_whole_timeout")
        self.assertEqual(meta["statuses"]["candidate"], "ok")
        self.assertEqual(meta["statuses"]["interactor"], "ok")
        self.check_processes_stopped(pids)
        original, _, valid, probe, output, options = self.shrink_hooks()
        proposer, pids = self.child_fixture("shrink_child")
        self.run_tool(SHRINK, *options, "--propose", proposer, "--timeout", 0.3, code=2)
        self.assertEqual(output.read_bytes(), original.read_bytes())
        self.check_processes_stopped(pids)

    @unittest.skipUnless(os.name == "posix", "SIGINT fixtures need POSIX")
    def test_process_interrupt_cleanup_and_shrink_progress(self):
        for tool in (STRESS, SHRINK):
            program, pids = self.child_fixture("interrupt_" + tool.stem)
            if tool == STRESS:
                args = [self.gen, program, self.good, "--count", 1, "--artifacts", self.artifacts]
            else:
                original, _, valid, probe, output, options = self.shrink_hooks()
                args = [*options, "--propose", program]
            command = [sys.executable, str(tool), *map(str, args)]
            self.last_command = command
            process = subprocess.Popen(command, cwd=self.cwd, stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE, text=True, start_new_session=True)
            try:
                deadline = time.monotonic() + 5
                while not pids.exists() and process.poll() is None and time.monotonic() < deadline:
                    time.sleep(0.01)
                self.assertTrue(pids.exists(), f"fixture did not start: {command}")
                process.send_signal(signal.SIGINT)
                out, err = process.communicate(timeout=5)
                self.assertEqual(process.returncode, 130, f"command={command}\nstdout={out}\nstderr={err}")
                self.check_processes_stopped(pids)
                if tool == SHRINK:
                    self.assertEqual(output.read_bytes(), original.read_bytes())
            finally:
                if process.poll() is None:
                    os.killpg(process.pid, signal.SIGKILL)
                process.communicate()

    def test_stress_seeded_token_oracles(self):
        rng = random.Random(SEED)
        for case in range(12):
            values = [rng.randrange(-2**100, 2**100) for _ in range(rng.randrange(0, 20))]
            expected = " ".join(map(str, values)) + "\n"
            spaced = "\t" + " \n".join(map(str, values)) + " \n"
            self.gen.write_text(f"print({expected!r},end='')\n")
            self.bad.write_text(f"print({spaced!r},end='')\n")
            with self.subTest(seed=SEED, case=case, values=values):
                self.run_tool(STRESS, self.gen, self.bad, self.good, "--seed", SEED + case, "--count", 1)
                self.run_tool(STRESS, self.gen, self.bad, self.good, "--seed", SEED + case,
                              "--count", 1, "--exact", code=1)
                folder, _ = self.failure("wrong_answer")
                self.assertEqual((folder / "input.txt").read_text(), expected)

    def test_stress_seeded_shrink_histories(self):
        rng = random.Random(SEED)
        original, proposer, valid, probe, output, options = self.shrink_hooks()
        for case in range(6):
            proposals = [rng.randrange(1, 35) for _ in range(12)]
            original.write_text("x" * 40)
            proposer.write_text(f"import sys\nsys.stdout.write('x'*{proposals!r}[int(sys.argv[2])])\n")
            valid.write_text("import sys\nfrom pathlib import Path\nn=len(Path(sys.argv[1]).read_bytes())\n"
                             "sys.exit(0 if n%2==0 else 1)\n")
            probe.write_text("import sys\nfrom pathlib import Path\nn=len(Path(sys.argv[1]).read_bytes())\n"
                             "print('failure:A' if n%4==0 else 'failure:B')\n")
            expected = min([40] + [n for n in proposals if n % 4 == 0])
            with self.subTest(seed=SEED, case=case, proposals=proposals):
                self.run_tool(SHRINK, *options, "--max-steps", len(proposals))
                self.assertEqual(output.read_bytes(), b"x" * expected)
                self.assertEqual(original.read_bytes(), b"x" * 40)
                output.unlink()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--mode", choices=("quick", "full", "stress"), default=os.environ.get("CP_TEST_MODE", "full"))
    parser.add_argument("--seed", type=int, default=int(os.environ.get("CP_TEST_SEED", "42")))
    args = parser.parse_args()
    MODE, SEED = args.mode, args.seed
    names = unittest.defaultTestLoader.getTestCaseNames(ContestTools)
    names = [name for name in names if (MODE == "stress" or not name.startswith("test_stress_"))
             and (MODE != "quick" or name in QUICK_TESTS)]
    print(f"contest fixtures: mode={MODE} seed={SEED} tests={len(names)}; "
          "failed fixture directories and commands are retained", flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(unittest.TestSuite(ContestTools(name) for name in names))
    raise SystemExit(not result.wasSuccessful())
