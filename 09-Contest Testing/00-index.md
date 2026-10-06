# Contest testing

Run the examples below from this folder, or pass the appropriate script paths from another directory. These tools require POSIX (Linux/macOS), Python 3.10+ and, for C++ sources, a GNU++20 compiler. `01-stress.py` runs one seed at a time. A generator takes the decimal seed as its first argument and writes the **entire stdin input** to stdout. Pass `.py`, `.cpp`, or executable files; C++ is compiled before the case loop in a temporary directory. The numbered templates are editable examples, not required protocol components.

For ordinary batch testing, `09-quick.py` exposes the same three positional arguments with only seed/count/timeout/artifact options and token comparison. It delegates to `01-stress.py`, including its detailed statuses, failure bundles and process cleanup. Keep both files together. Use the full runner for replay, custom judging, compiler settings and other modes.

```bash
python3 01-stress.py 02-gen.py optim.cpp brute.py --count 1000 --seed 42
python3 01-stress.py 02-gen.py optim.cpp brute.py --checker 04-checker.py
python3 01-stress.py 02-gen.py optim.cpp brute.py --replay /tmp/contest-stress-artifacts/CASE/input.txt
```

Batch mode compares whitespace-separated byte tokens by default; `--exact` compares all output bytes. A checker receives three paths: `input candidate_output reference_output`. Exit 0 accepts, 1 reports a wrong answer, and any other exit or timeout is a checker error. With `--checker`, the reference is optional; its output file is empty when omitted. If supplied, the reference must finish successfully before the candidate is judged. With `--replay saved.in --expected saved.ans` and no reference program, the saved answer file is the reference output, so sample tests need no brute; it works with token, `--exact` or checker comparison. `--exact` and `--checker` are mutually exclusive.

`--input-validator` (every mode, including replay) receives the input path before any program runs: exit 0 means valid, 1 reports `invalid_input`, and anything else or a timeout reports `input_validator_error`. Both are inconclusive generator/infrastructure failures, never contestant verdicts.

```bash
python3 01-stress.py unused solution.cpp --mode interactive --interactor 05-interactor.py --replay hidden.in
python3 01-stress.py 02-gen.py heuristic.cpp baseline.cpp --mode scored --validator 06-validator.py --scorer 07-scorer.py --objective maximize --count 100
```

Interactive mode gives the interactor the hidden input **file path** as its first argument. The contestant sees only bytes sent through the interactor's stdout; its stdout goes to the interactor's stdin. Exit 0 from both means accepted; interactor exit 1 means rejected, and other exits are interactor errors. The runner records both raw directions and a timestamped base64 transcript. `--whole-timeout` and `--idle-timeout` bound a stalled exchange, including inherited pipes held open after their parent exits. Replay uses the same saved hidden input; the exchanged bytes depend on deterministic interactor/contestant behavior, and timestamps may differ.

For the example above, `hidden.in` contains one integer in `[-100,100]`. `05-interactor.py` prints `guess`, accepts up to eight `? y` queries, and replies `<`, `=` or `>` comparing the hidden value with `y`. Send the final integer on its own line. The interactor rejects malformed messages, early EOF and a ninth query. Both sides must flush each message. This example's hidden format differs from `02-gen.py`; use a matching generator for randomized interactive testing.

Scored mode's validator receives `input output` paths: exit 0 means valid, 1 invalid, other statuses mean a tool error. The scorer receives the same paths and must print one finite decimal number. `--objective maximize|minimize` controls the sign of `gap`: positive means the candidate trails the optional reference. Different valid scores are reported, never called wrong answers; there is no stop-on-regression policy. Without a reference, the candidate score is printed alone. Decimal parsing preserves score digits; widely separated exponents may round the displayed gap while preserving its sign. Scores/gaps outside the Decimal implementation's representable range report `scorer_error`.

`--count 0` (the default) runs until failure or Ctrl-C. Timeouts must be finite and positive. `--timeout`, `--python`, `--cxx`, and `--cxxflags` control process limits and compilation (each compiler invocation has a 60-second limit). Paths containing spaces work when quoted in the shell. The first failing seed gets a unique folder under `/tmp/contest-stress-artifacts` by default (`--artifacts` changes it), with input, outputs, stderr, source paths, effective command arguments, comparison/configuration, and statuses. Replay that input with `--replay`; the generator is ignored. Builds stay in temporary storage and are never written beside the source. Exit 0 means all requested cases passed; 1 means a saved case failure (including infrastructure failures), 2 means setup/artifact error, and 130 means interrupted. Inspect the printed kind and `case.json` to distinguish contestant failure from generator/reference/tool failure.

The common engine kills child process groups and closes pipes on completion, timeout and interruption. Programs deliberately starting a new session can escape that group. Output is buffered in memory in batch mode and saved to disk in interactive mode; there is no output or memory quota. These are trusted local testing tools, not an isolation sandbox. Ctrl-C cleans up but does not save a partially judged case; replay concerns completed failure bundles.

`08-shrink.py` is an optional, separate driver for **problem-specific** shrinking. Keep it beside `01-stress.py`, whose process helper it shares. Hooks are Python files or executables: `--propose` receives the current input path and zero-based step, emits proposed bytes, or exits 3 to skip; `--valid` receives the input path and exits 0 for valid or 1 for invalid; `--probe` receives the input path, exits 0 and emits a nonblank failure-class/signature. Other exits/timeouts are hook errors. Signatures compare **exact bytes, including whitespace**. A probe must encode success as a different signature from the original failure.

The helper validates the original before probing it, accepts only strictly shorter valid proposals with the same signature, and stops after `--max-steps` attempts. All hook calls have `--timeout`; there are at most `2 + 3 * max_steps` calls. The original stays unchanged and the reduced file is created exclusively (`--output` chooses its path). Once the baseline is established, a hook error or Ctrl-C still saves the best accepted case, exiting 2 or 130. It does not delete text generically or guarantee a globally minimal result. Example:

```bash
python3 08-shrink.py failing.in --propose my_proposer.py --valid my_input_validator.py --probe my_failure_probe.py --max-steps 100
```

## Coverage and extension inventory

P001/SUP01 verification is recorded in [the package evidence](10-verification.md), with the feature-to-test map and scoped follow-up work. The examples remain small demonstrations, not a comprehensive generator/checker library for every algorithm family.

The [integer and style maintenance record](10-verification.md#integer-and-style-maintenance-2026-09-27) covers the standalone generator's exact local aliases, applicable C++ fixtures, and fresh full-suite/reproducibility checks.

| Files | Owned features | Status and evidence |
|---|---|---|
| `01-stress.py`, `09-quick.py` | Sequential seeds, Python/C++/executables, token/byte/custom comparison, input validator, expected-answer replay, error classes, unique bundles, replay, child cleanup, reduced quick CLI | **Verified**, 2026-10-06 re-audit; [P001 evidence](10-verification.md) |
| `01-stress.py`, `05-interactor.py` | Hidden input, bidirectional relay/transcript, EOF/backpressure, total/idle limits, bounded query example | **Verified**; same evidence |
| `01-stress.py`, `06-validator.py`, `07-scorer.py` | Validity versus tool errors, finite Decimal scores, both objectives, optional reference, permutation example | **Verified**; same evidence |
| `08-shrink.py` | Valid baseline, shorter valid proposals, exact signatures, skip/errors/timeouts, bounded attempts, preserved progress | **Verified**; same evidence |
| `02-gen.py`, `03-gen.cpp`, `04-checker.py` | Reproducible complete integer-array inputs; exact token checker without float conversion | **Verified**; same evidence |

Both generators emit `1 <= n <= 20` and `n` integers in `[-100,100]`. Python accepts signed integer seeds; the C++ example accepts decimal seeds in `[0,2^64-1]`. Repeating a seed in the same generator reproduces its case; the two generators need not agree. The checker compares tokens exactly. The scored example treats the first input token as nonnegative `n`, validates a permutation of `1..n` (empty is valid when `n=0`), then counts increasing adjacent pairs.

Planned problem-specific additions, made only when a task needs them:

- Seeded generators for valid graph/tree/multigraph instances, integer/field/geometry boundary cases, strings and stateful operation histories; combine exhaustive tiny cases, degenerate shapes and randomized distributions. A generator must enforce its domain; `--input-validator` checks it independently.
- Checkers for nonunique paths, cuts, matchings, decompositions and constructive outputs; numerical checkers with an explicit absolute/relative/error-bound policy. Exact integer tokens must stay exact.
- Additional problem-specific interactor protocols and scoring policies as their problems require; the supplied bounded guessing/permutation examples cover the common lifecycle.
- Shrink hooks that preserve graph/tree structure, algebraic domains and legal rollback/persistence histories. The current helper's progress metric is strictly decreasing **input byte length**, with bounded attempts and per-hook timeouts; arbitrary semantic metrics, a separate total-time limit and generic interactive/scored reducers are not implemented.

Keep these adapters short and separate. Algorithm regression suites and performance benchmarks remain in `96-Local Testing`; successful problem stress does not verify an entire library family. See [the completeness audit](../00-Guidelines/16-inventory-audit.md) for the expanded workload inventory.
