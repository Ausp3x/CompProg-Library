# P001 / SUP01 verification

Scope: the selected SUP01 row owns concrete gaps in the existing stress, quick,
interactive, scored and shrinking protocols and their fixtures. It does not own
the future graph, algebra, geometry or stateful generator/checker corpus.

## Changes and correctness arguments

- The common process helper kills its process group, reaps its leader and closes
  pipes after success, failure, timeout and interruption. Quick testing and shrink
  hooks reuse it, eliminating their weaker duplicate subprocess handling. Timers
  reject NaN/infinity; zero-count iteration is actually unbounded. Compiler runs
  have a separate 60-second timeout. Invalid mode/hook combinations are rejected;
  `--exact` with `--checker` exits 2 at setup.
- Batch testing generates one byte string and gives it unchanged to both programs.
  A failing reference prevents candidate judging. Token comparison splits bytes,
  never converts integers to floating point; byte comparison preserves all bytes.
  Checker exit 1 is a wrong answer, while other failures remain tool errors.
- Interactive relays handle partial writes and backpressure with nonblocking
  pipes. Both process completion and pipe EOF are required for acceptance, so a
  descendant holding a pipe cannot cause a false pass. Total/idle timeouts also
  bound relays blocked on a peer. Threads stop before transcript files close;
  relay failures cannot silently disappear. Hidden bytes go only to the
  interactor's file argument. Recorded chunks are transport chunks, not messages.
- Scored outputs must pass validation before scoring. Scores are finite Decimals;
  the gap uses enough precision to preserve the ordering of the supplied values.
  Widely separated exponents may round the displayed magnitude. Decimal overflow
  or underflow beyond its backend range is a tool error, never a zero-gap pass.
  Both objective directions report quality differences without failing a run.
- Shrinking first establishes a valid baseline and a nonblank signature. Each
  accepted replacement is valid, strictly shorter in bytes, and has the identical
  signature, including whitespace. Induction preserves that invariant; bounded
  attempts and per-hook timeouts bound the search. It is a problem-specific local
  reduction, not a proof of global minimality. Errors/interruption retain the last
  accepted bytes, without overwriting the original or an existing output.
- Templates distinguish malformed hidden data/tool I/O from invalid contestant
  output. The guessing example enforces eight comparisons and explicit flushing;
  all 201 hidden values can be solved by binary search. Permutation validity uses
  length, range and uniqueness; those conditions are necessary and sufficient,
  including the empty permutation. Generator seed parsing is explicit and the
  complete emitted arrays satisfy their documented bounds. `03-gen.cpp` stays
  standalone with a local `ulng = uint64_t` seed, keeping the full `[0,2^64-1]`
  domain that `lng` would halve; the numeric C++ candidate fixtures use local
  `lng = int64_t` aliases. Its `1 + int(rng() % 20)` cast is value-preserving
  (`rng() % 20 < 20`).
- `--input-validator` runs once per case on the exact input bytes (generated or
  replayed), in every mode, after a successful generator and before any program
  starts. Exit 0 continues; exit 1 is `invalid_input`; other exits/timeouts are
  `input_validator_error`. Both save the usual bundle (input, validator streams,
  effective command with the input path) and are infrastructure failures, never
  contestant verdicts. Interactive hidden input is therefore never shown to the
  contestant when invalid: the candidate is not launched. The validator run is
  always kept in `statuses` and streams, and the input-path command is recorded
  only when the validator ran.
- `--expected FILE` (batch, `--replay`, no reference program) reads the answer
  once at setup and substitutes it as a successful reference output. Token, byte
  and checker comparison are then unchanged code paths; the bundle stores it as
  `reference.out` and its path in `config.expected`. A missing file exits 2.

Failure bundles preserve source paths, compiler/interpreter settings, working
directory, replay path, effective hook arguments, statuses, streams and hidden
input. Interactive failures additionally preserve both raw directions and a
timestamped base64 transcript. Temporary binary/hook-input paths in command
metadata are historical: use `--replay` with source programs to rebuild them.

## Feature-to-test map

The maintained entry is
[`01-contest_tester.py`](<../../96-Local Testing/00-Tools/01-contest_tester.py>).
Names below omit the common `test_` prefix; related methods supplement each row.

| Files / features | Named fixture coverage |
|---|---|
| `01-stress.py`: byte/token comparison, exact large integers, stderr, candidate/reference/generator errors | `batch_pass_exact_and_cpp`, `batch_tokens_bytes_large_integers_and_stderr`, `batch_error_classes`, `batch_and_quick_role_timeouts_and_launch_errors` |
| `01-stress.py`: unique bundles, replay without generator, hook argv, configuration | `mismatch_replay_and_artifact_metadata`, `mismatch_replay_ignores_generator_and_unique_artifacts`, `configuration_rejects_invalid_limits_and_missing_programs` |
| `01-stress.py`: C++ compiled once before multiple cases, custom compiler/interpreter/flags, existing executables, failed builds | `cpp_compile_once_flags_executables_and_failures` |
| `01-stress.py`, `04-checker.py`: optional reference, checker paths/statuses, reference failure precedence, exact checker template | `checker_without_reference`, `checker_status_paths_and_reference_failure_order`, `checker_template_exact_integer_tokens_and_errors` |
| `02-gen.py`, `03-gen.cpp`: reproducibility, bounds, uint64 endpoints, invalid seeds | `generator_templates_seeded_domain_and_cpp` |
| `01-stress.py`, `05-interactor.py`: hidden input, transcript reconstruction, total/idle timeouts, rejection/error, 256 KiB backpressure, query budget, malformed/EOF input | `interactive_transcript_rejection_and_idle`, `interactive_protocol_error_classes_and_total_timeout`, `interactive_large_transfer_and_template_queries`, `interactive_template_rejection_and_hidden_errors` |
| `01-stress.py`, `06-validator.py`, `07-scorer.py`: objectives, optional reference, validity/tool errors/timeouts, Decimal precision, nonfinite/invalid scores, permutation boundaries | `scored_valid_gap_and_errors`, `scored_objectives_precision_and_finite_output`, `scored_validator_and_scorer_statuses`, `scored_templates_boundaries_and_error_contract` |
| `08-shrink.py`: baseline validation, signature bytes, decreasing size, skipped proposals, bounded attempts, no overwrite, hook errors retaining progress | `shrink_validity_and_signature`, `shrink_exact_signature_size_skip_and_attempt_bound`, `shrink_initial_failures_and_invalid_limits`, `shrink_hook_failures_preserve_last_accepted_case` |
| `09-quick.py`: reduced interface and common engine integration | `quick_batch_entry`, plus build/configuration/timeout/process fixtures above |
| All drivers: timeout/interrupt descendant cleanup and retained shrink result | `process_timeout_cleans_descendants_in_all_drivers`, `process_interrupt_cleanup_and_shrink_progress` |
| `01-stress.py`: input validator in every mode (valid/invalid/tool error), expected-answer replay with token/byte/checker comparison, `--exact`/`--checker` exclusion | `input_validator_and_expected_answer_replay`, `configuration_rejects_invalid_limits_and_missing_programs` |
| Extended independent seeded oracles | `stress_seeded_token_oracles`, `stress_seeded_shrink_histories` |

Quick runs eight smoke/regression methods. Full runs 28 deterministic methods.
Stress runs those 28 plus 12 seeded exact/token comparison cases and six seeded
shrinking histories, each with 12 proposals and an independently computed result.
All subprocess fixtures run from an unrelated directory with spaces. Harness
rules: [05-testing.md](../../00-Guidelines/05-testing.md).

## Commands and results (2026-10-06)

| Command (Linux x86_64, CPython 3.14, GCC 16.2, cwd `/tmp`) | Result |
|---|---|
| `python3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode quick` | PASS, 8 tests |
| `python3 -O '96-Local Testing/00-Tools/01-contest_tester.py' --mode full` | PASS, 28 tests |
| `python3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode stress --seed 11` | PASS, 30 tests |
| `python3 '96-Local Testing/01-run.py' --mode stress --seed 43 --rounds 1 --filter 00-Tools/01-contest --no-integration` | PASS |
| `g++ -std=gnu++20 -O2 -Wall -Wextra -Wconversion 03-gen.cpp` | 0 warnings |
| Pre-edit vs final generator (`-O2`) and final under `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined` | byte-identical stdout, stderr and exit codes for seeds `0,1,42,2^63-1,2^63,2^64-1` and 7 invalid argv cases; identical md5 over seeds `0..2000`; no sanitizer output |
| `ast.parse(..., feature_version=(3,10))` on the 8 toolkit files and the tester | PASS, 9 files |

PyPy 3.12.14 (PyPy 8.0.0): the tester run under
`pypy3` also launches every tool with `pypy3`, because the tools default to the
running interpreter. Commands from cwd `/tmp`:

| Command | Result |
|---|---|
| `pypy3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode quick` | PASS, 8 tests in 12.7 s |
| `pypy3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode full` | PASS, 28 tests in 80.8 s |
| `pypy3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode stress --seed 11` | PASS, 30 tests in 109.0 s |

CPython 3.10.22 (uv-managed, `uv python find 3.10`) exercises the Python floor
end to end; the tools again inherit the
interpreter. Commands from cwd `/tmp`, with `P=$(uv python find 3.10)`:

| Command | Result |
|---|---|
| `$P '96-Local Testing/00-Tools/01-contest_tester.py' --mode quick` | PASS, 8 tests in 2.7 s |
| `$P -O '96-Local Testing/00-Tools/01-contest_tester.py' --mode full` | PASS, 28 tests in 15.3 s |
| `$P '96-Local Testing/00-Tools/01-contest_tester.py' --mode stress --seed 11` | PASS, 30 tests in 19.2 s |

No benchmark: no algorithm or threshold is involved.

## Provenance

New support files from the [2026-09-27 reorganization](../../00-Guidelines/History/2026-09-27-migration.md);
no legacy code transferred and no external code imported. Sources consulted:
[00-sources.md](00-sources.md).

## Limits and follow-up ownership

- POSIX process-group semantics are required. Programs deliberately starting new
  sessions can escape cleanup. Output/resource quotas, hostile-code isolation,
  Windows process supervision and saving interrupted partially judged cases are
  outside this short local toolkit's contract. Batch output occupies memory;
  interactive transcripts occupy disk proportional to observed traffic.
- Templates and hooks remain problem-specific. A generator enforces its input
  domain; `--input-validator` can check it independently. Nonunique
  witness checkers and numerical tolerance policies must be supplied explicitly.
  There is no implicit conversion of exact integer answers to floating point.
- New algorithm packages own their graph/tree/multigraph, numeric-boundary,
  string and legal-history generators/checkers. They also own structure-preserving
  shrink hooks and interactive/scored probes when their problem demands them.
  Arbitrary semantic shrink metrics, a separate global shrink timeout and generic
  reducers remain conditional planned extensions in the folder index. Nothing in
  P001 claims those future adapters are implemented.
- Only Linux was exercised; macOS execution is not available here.

## History

- 2026-09-27: P001/SUP01 completed; quick (8), full (27) and stress (29) passed on CPython 3.14.7 and GCC 16.2.1, with the sanitized C++ generator and a 201-case interactive binary-search check.
- 2026-09-27: integer and style maintenance (`ulng`/`lng` aliases, generator layout); full suite passed and generator output stayed byte-identical.
- 2026-09-27: post-migration defaults review, no source edit; the generator and C++ fixture tests passed.
- 2026-10-06: re-audit; generator brace and `-Wconversion` fix, `--exact`/`--checker` rejection, `--input-validator` and `--expected` added, two review findings fixed; quick, full (28) and stress (30) passed on CPython 3.14, CPython 3.10 and PyPy 3.12.
