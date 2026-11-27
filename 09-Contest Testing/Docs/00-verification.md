# P001 / SUP01 Verification

Scope: the SUP01 row owns concrete gaps in the existing stress, quick, interactive, scored and shrinking protocols and their fixtures. It does not own the future corpus of graph, algebra, geometry or stateful generators and checkers.

## Contracts

- Processes: the common process helper kills its process group, reaps its leader and closes pipes after success, failure, timeout and interruption. Quick testing and shrink hooks reuse it, which removed their weaker duplicate subprocess handling. Timers reject NaN and infinity, and a zero count really iterates without bound. Compiler runs have their own 60-second timeout. Invalid combinations of mode and hook are rejected; `--exact` with `--checker` exits 2 at setup.
- Batch testing generates one byte string and gives it unchanged to both programs. A failing reference prevents judging the candidate. Token comparison splits bytes and never converts integers to floating point; byte comparison preserves every byte. Checker exit 1 is a wrong answer, and any other checker failure remains a tool error.
- Interactive relays handle partial writes and backpressure with nonblocking pipes. Acceptance requires both process completion and pipe EOF, so a descendant holding a pipe cannot cause a false pass. Total and idle timeouts also bound relays blocked on a peer. Threads stop before the transcript files close, so a relay failure cannot disappear silently. Hidden bytes go only to the interactor's file argument. Recorded chunks are transport chunks, not messages.
- Scored outputs must pass validation before scoring. Scores are finite Decimals, and the gap uses enough precision to preserve the order of the supplied values; widely separated exponents may round the displayed magnitude. A Decimal overflow or underflow beyond the backend's range is a tool error, never a zero-gap pass. Both objective directions report quality differences without failing a run.
- Shrinking first establishes a valid baseline and a nonblank signature. Each accepted replacement is valid, strictly shorter in bytes and has the identical signature, whitespace included; by induction every accepted case keeps that invariant. Bounded attempts and per-hook timeouts bound the search. It is a problem-specific local reduction, not a proof of global minimality. An error or interruption keeps the last accepted bytes, without overwriting the original or an existing output.
- Templates tell malformed hidden data or tool I/O apart from invalid contestant output.
  - The guessing example enforces eight comparisons and explicit flushing; binary search solves all 201 hidden values.
  - Permutation validity checks length, range and uniqueness, which are necessary and sufficient, the empty permutation included.
  - Generator seed parsing is explicit, and the complete emitted arrays satisfy their documented bounds.
  - `03-gen.cpp` stays standalone with a local `ulng = uint64_t` seed, keeping the full `[0,2^64-1]` domain that `lng` would halve; the numeric C++ candidate fixtures use local `lng = int64_t` aliases. Its `1 + int(rng() % 20)` cast preserves the value, since `rng() % 20 < 20`.
- `--input-validator` runs once per case on the exact input bytes, generated or replayed, in every mode, after a successful generator and before any program starts. Exit 0 continues, exit 1 is `invalid_input`, and any other exit or a timeout is `input_validator_error`. Both save the usual bundle (input, validator streams, the effective command with the input path) and are infrastructure failures, never contestant verdicts. An invalid interactive hidden input is therefore never shown to the contestant, because the candidate is not launched. The validator run is always kept in `statuses` and the streams; the input-path command is recorded only when the validator ran.
- `--expected FILE` (batch mode with `--replay` and no reference program) reads the answer once at setup and substitutes it as a successful reference output. Token, byte and checker comparison then run their unchanged code paths. The bundle stores the answer as `reference.out` and its path in `config.expected`. A missing file exits 2.
- Output limits: `invoke` multiplexes stdin, stdout and stderr through one selector. It writes stdin in `PIPE_BUF` chunks only once the pipe is writable, so it never blocks and cannot deadlock against a child filling its output. Each stream holds at most `--output-limit` bytes: the chunk that passes the limit is cut to it, the process group is killed and the status is `output limit`. At the deadline the group is killed, and the pipes drain for at most one more second, so the bytes written before the kill stay, as `communicate()` kept them. After both outputs reach EOF the wait is bounded by the time left. Interactive mode reads both stderr streams with the relay function, each in its own thread, and counts each of the four streams, so its files also stop at the limit. The chunk cut at the limit is written and logged like any other, so a direction rebuilt from the transcript still equals its `.bin` file. After the processes are killed and reaped, the stderr threads get up to one second to read their pipes to EOF before the relays stop, so a program's last stderr bytes reach its file. The limit flag is checked in the supervision loop and again after it, because a relay closing its pipes at the limit can end both processes first. Acceptance still waits only for the two relays' EOF, never for a stderr pipe a descendant holds.
- `--memory-limit` sets `RLIMIT_AS` in the child between fork and exec (`preexec_fn`), so the exec'd program and its descendants inherit it. It applies to the candidate and the reference, never to the generator, the interactor or a hook. The interactive processes start before any relay thread, so the hook never runs in a threaded parent. A refused allocation ends the program with its own failure status, judged as its crash. A value of `2^63` bytes or more, or above the hard limit the runner inherited (`ulimit -Hv`), is rejected at setup with exit 2; a hook failure at launch that still occurs is a `launch error`, never a traceback. On macOS, where `RLIMIT_AS` may be refused or not enforced, the same launch error or an unenforced limit results (untested).
- Ctrl-C during a case prints `interrupted at seed=N` (`interrupted` alone in replay mode or before the first case) and saves no partially judged case: the seed regenerates its input and a replay rereads its file, so nothing is lost.
- `08-shrink.py --total-timeout` fixes one deadline at start. Each hook call gets `min(--timeout, time left)`; a call refused for lack of time, or one timed out with less than `--timeout` left, ends the run as expiry, never as a hook error. Expiry during the search keeps the best accepted case, which by the shrinking invariant is valid and has the original signature. Expiry before the baseline is established exits 2 without writing a case.
- Process model: POSIX process groups are required, and a program that deliberately starts a new session leaves the group and can escape cleanup. Windows process supervision and isolation of hostile code are outside this trusted local toolkit. Every other resource limit (stack size, CPU time, open files) is inherited from the shell that starts the runner, so `ulimit -s unlimited` or `ulimit -s 262144` before a run sets the stack as a judge would, for every process. Batch outputs occupy memory, each stream at most the output limit, and about twice that while the final copy is made; interactive streams occupy disk up to the limit.
- Templates and hooks are problem-specific. A generator enforces its input domain, and `--input-validator` can check it independently. Checkers for nonunique witnesses and numerical tolerance policies are supplied explicitly; exact integer answers are never converted to floating point implicitly.
- Failure bundles preserve source paths, compiler and interpreter settings, the working directory, the replay path, effective hook arguments, statuses, streams and hidden input. Interactive failures also preserve both raw directions and a timestamped base64 transcript. Temporary paths of binaries and hook inputs in the command metadata are historical: use `--replay` with the source programs to rebuild them.

## Feature-to-Test Map

The maintained entry is [`01-contest_tester.py`](<../../96-Local Testing/00-Tools/01-contest_tester.py>). Names below omit the common `test_` prefix; related methods supplement each row.

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
| All drivers: timeout/interrupt descendant cleanup, the interrupted seed after a passing case and retained shrink result | `process_timeout_cleans_descendants_in_all_drivers`, `process_interrupt_cleanup_and_shrink_progress` |
| `01-stress.py`: input validator in every mode (valid/invalid/tool error), expected-answer replay with token/byte/checker comparison, `--exact`/`--checker` exclusion | `input_validator_and_expected_answer_replay`, `configuration_rejects_invalid_limits_and_missing_programs` |
| `01-stress.py`: output limit per stream and role (candidate stdout and stderr, generator, reference, checker, both interactive directions with the transcript, both interactive stderr streams, exactly-at-limit pass), memory limit on candidate and reference only (generator and interactor exempt, default unlimited), invalid limits and one above the inherited hard limit; a leader closing its outputs and a descendant leaving the group with stdout both time out within bound | `output_and_memory_limits`, `process_timeout_cleans_descendants_in_all_drivers` |
| `08-shrink.py`: total timeout ending the search with retained progress, a hook call capped at the time left before the baseline, invalid values | `shrink_total_timeout_keeps_progress` |
| Extended independent seeded oracles | `stress_seeded_token_oracles`, `stress_seeded_shrink_histories` |

Quick runs eight smoke and regression methods, and full runs 30 deterministic methods. Stress runs those 30 plus 12 seeded exact and token comparison cases and six seeded shrinking histories, each with 12 proposals and an independently computed result. All subprocess fixtures run from an unrelated directory whose path has spaces. Harness rules: [05-testing.md](../../00-Guidelines/05-testing.md).

## Commands and Results

Latest run, 2026-11-27 (`/package P001`, experiment none), Linux x86_64, from the worktree root. The CPython 3.14 runs used four parallel processes and the CPython 3.10 runs three, while PyPy ran alone. A tester run under another interpreter also launches every tool with it, because the tools default to the running interpreter.

| Command (CPython 3.14.8, GCC 16.2.1 unless named) | Result |
|---|---|
| `python3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode quick` | PASS, 8 tests in 10.2 s |
| `python3 -O '96-Local Testing/00-Tools/01-contest_tester.py' --mode full` | PASS, 30 tests in 56.2 s |
| `CXX=g++-14 python3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode full` (GCC 14.4.1) | PASS, 30 tests in 55.4 s |
| `python3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode stress --seed 11` | PASS, 32 tests in 66.8 s |
| `$P '96-Local Testing/00-Tools/01-contest_tester.py' --mode quick` (CPython 3.10.22, `P=$(uv python find 3.10)`) | PASS, 8 tests in 5.1 s |
| `$P -O '96-Local Testing/00-Tools/01-contest_tester.py' --mode full` | PASS, 30 tests in 33.7 s |
| `$P '96-Local Testing/00-Tools/01-contest_tester.py' --mode stress --seed 11` | PASS, 32 tests in 39.3 s |
| `pypy3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode quick` (PyPy 8.0.1, Python 3.12.15) | PASS, 8 tests in 19.9 s |
| `pypy3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode full` | PASS, 30 tests in 160.5 s |
| `pypy3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode stress --seed 11` | PASS, 32 tests in 201.8 s |

`g++ -std=gnu++20 -O2 -Wall -Wextra -Wconversion 03-gen.cpp` gave 0 warnings, and the `-O2` build and the `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined` build gave byte-identical stdout, stderr and exit codes. The arguments were the seeds `0,1,42,2^63-1,2^63,2^64-1`, 7 invalid ones (`-1`, `2^64`, `abc`, empty, `1 2`, `1x`, ` 1`) and none at all, and both builds gave the same md5 `74a27bf6…` over seeds `0..2000`, with no sanitizer output. `invoke` launched a PyPy generator as fast as the replaced `communicate()` version, with medians of 0.077 s and 0.075 s over 10 launches. The fixture timeouts were raised from 0.1–0.3 s to 0.5–1 s, and the interactive whole and idle timeouts from 0.3 and 1 s to 1 and 3 s, because PyPy's 0.07 s start-up timed out a 0.1 s generator when six suites ran at once and later missed a 0.3 s interactive deadline.

No benchmark applies: no algorithm or threshold is involved.

## Provenance

The support files are new since the [2026-09-27 reorganization](../../00-Guidelines/History/2026-09-27-migration.md). No legacy code was transferred and no external code imported. Sources consulted: [00-sources.md](00-sources.md).

## Limits and Handoffs

- The folder index's planned problem-specific additions are made only when a task needs them, by the package whose problem needs them: generators and checkers for graphs, trees, multigraphs, numeric boundaries, strings and legal histories, structure-preserving shrink hooks, semantic shrink metrics and generic interactive or scored reducers (user decision 2026-11-27).
- Only Linux was exercised; macOS stays in the stated platforms untested, since no macOS machine is available (user decision 2026-11-27).

## History

- 2026-09-27: P001/SUP01 completed; quick (8), full (27) and stress (29) passed on CPython 3.14.7 and GCC 16.2.1, with the sanitized C++ generator and a 201-case interactive binary-search check.
- 2026-09-27: integer and style maintenance (`ulng`/`lng` aliases, generator layout); full suite passed and generator output stayed byte-identical.
- 2026-09-27: post-migration defaults review, no source edit; the generator and C++ fixture tests passed.
- 2026-10-06: re-audit; generator brace and `-Wconversion` fix, `--exact`/`--checker` rejection, `--input-validator` and `--expected` added, two review findings fixed; quick, full (28) and stress (30) passed on CPython 3.14, CPython 3.10 and PyPy 3.12.
- 2026-11-10: P274 whole-library runs: `01-contest_tester.py` quick (8) and full (28) passed on g++ and g++-14.
- 2026-11-27: limits recheck: output and memory limits, the interrupted seed and the shrink total timeout added per user decisions; review: 8 findings fixed (memory-limit setup crash, lost interactive stderr tail, a contract item, transcript cut chunk, 4 test gaps); quick (8), full (30) and stress (32) passed on CPython 3.14, CPython 3.10 and PyPy 3.12, full on g++-14.
