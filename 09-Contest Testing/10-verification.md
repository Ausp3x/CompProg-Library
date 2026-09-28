# P001 / SUP01 verification

Scope: the selected SUP01 row owns concrete gaps in the existing stress, quick,
interactive, scored and shrinking protocols and their fixtures. It does not own
the future graph, algebra, geometry or stateful generator/checker corpus. There
are no prerequisite implementation packages. **Verified and complete on
2026-09-27**, with no remaining owned implementation or verification gaps.

## Changes and correctness arguments

- The common process helper kills its process group, reaps its leader and closes
  pipes after success, failure, timeout and interruption. Quick testing and shrink
  hooks reuse it, eliminating their weaker duplicate subprocess handling. Timers
  reject NaN/infinity; zero-count iteration is actually unbounded. Compiler runs
  have a separate 60-second timeout. Invalid mode/hook combinations are rejected.
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
  complete emitted arrays satisfy their documented bounds.

Failure bundles preserve source paths, compiler/interpreter settings, working
directory, replay path, effective hook arguments, statuses, streams and hidden
input. Interactive failures additionally preserve both raw directions and a
timestamped base64 transcript. Temporary binary/hook-input paths in command
metadata are historical: use `--replay` with source programs to rebuild them.

## Feature-to-test map

The maintained entry is
[`01-contest_tester.py`](<../96-Local Testing/00-Tools/01-contest_tester.py>).
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
| Extended independent seeded oracles | `stress_seeded_token_oracles`, `stress_seeded_shrink_histories` |

Quick runs eight smoke/regression methods. Full runs 27 deterministic methods.
Stress runs those 27 plus 12 seeded exact/token comparison cases and six seeded
shrinking histories, each with 12 proposals and an independently computed result.
The suite accepts `--mode`/`--seed` or the top-level runner's environment settings.
All subprocess fixtures run from an unrelated directory with spaces. Checks use
non-removable unittest oracles. Failures retain the fixture files, method/subtest,
seed, working directory and command; they are the smallest known reproducers,
not automatically minimized. Missing compilers produce an explicit skip.

## Execution evidence

On 2026-09-27: Linux x86_64, CPython 3.14.7, GCC 16.2.1; default C++ commands use
`-O2 -std=gnu++20`. These are correctness runs, not performance measurements.
No optional test was skipped on this host.

| Command / configuration | Result |
|---|---|
| `python3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode quick` (absolute script path, cwd `/tmp`) | PASS, 8 tests |
| `python3 -O '96-Local Testing/00-Tools/01-contest_tester.py' --mode full` (absolute script path, cwd `/tmp`) | PASS, 27 tests; assertions remain effective with optimization |
| `python3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode stress --seed 42` | PASS, 29 methods including the extended workloads |
| `python3 '96-Local Testing/01-run.py' --mode quick --filter 00-Tools/01-contest --no-integration` | PASS, 8 tests; discovery/environment integration |
| `python3 '96-Local Testing/01-run.py' --mode stress --seed 43 --rounds 1 --filter 00-Tools/01-contest --no-integration` | PASS, 29 methods |
| `ast.parse(..., feature_version=(3,10))` for all eight toolkit Python files and the fixture entry | PASS, 9 files; syntax compatibility only |
| C++ generator with `-std=gnu++20 -O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-omit-frame-pointer` | PASS for seeds `0,1,42,2^64-1`; rejected absent/empty/negative/overflow/junk/extra arguments with exit 2; independent output-domain checks |
| Independent interactive binary search over every hidden integer `-100..100` | PASS, 201 cases, at most eight comparisons each |
| Final targeted rerun of configuration, scored precision and descendant-cleanup methods after persisting the last regressions | PASS, 3 methods; includes incompatible hooks, tiny-gap underflow in both objectives and inherited pipes after both leaders exit |
| Final generator-template method after C++ style/qualification cleanup | PASS; deterministic bounds, seed endpoints and invalid arguments |

The sanitizer run first encountered LeakSanitizer's documented inability to run
under ptrace in this sandbox. The identical check was rerun outside the sandbox
with approval and passed. There was no sanitizer diagnostic from the generator.
Focused additional runner checks covered partial interactive launch and thread
startup failures. No online submission or algorithm-library verification is
claimed by this support package.

## Integer and style maintenance (2026-09-27)

Applied the revised [integer and style defaults](../00-Guidelines/03-cpp.md)
using the [completed-package maintenance workflow](../01-prompts.md#updating-a-completed-package-after-a-rule-change).
This section records fresh maintenance checks; the stress/sanitizer results above
remain the original completion evidence, not new runs for this change.

- `03-gen.cpp` stays standalone and defines only `using ulng = uint64_t;` locally.
  The seed uses `ulng`, preserving exact type identity and the full unsigned
  `[0,2^64-1]` domain. Unsigned storage is intentional here; `lng` would lose half
  that domain. `argc`, `n` and `i` remain `int`; generated lengths are at most 20.
  The bounded `rng() % 20` conversion remains safe without a warning-only cast.
- The two numeric C++ candidate fixtures use local `using lng = int64_t;` aliases
  in place of `long long`, retaining signed 64-bit input capacity on the supported
  platform. Their stream operations have no dependency on the old spelling's
  distinct type identity. The `<iostream>` fixture now includes `<cstdint>` for
  its alias. Underlying integer spellings remain only in these alias definitions.
- Generator declarations, parsing and generation are separated visually. The
  fixture bodies separate input from output, with consistent spacing and the
  established closing-brace style. The stream-copy fixture retains its single
  coherent output step. Each C++ program has only `main`, so there are no multiple
  methods to regroup or public operations to rename.

`std::from_chars`, `std::mt19937_64`, modulo mapping and random draw order remain
identical. Python conventions, public names and all tool protocols are preserved.
No Core dependency, legacy edit, shared-checklist edit or unrelated session change
was made. There are no remaining maintenance gaps or additional style exceptions.
This spelling/layout update makes no performance claim and needs no new benchmark.

Fresh checks on Linux x86_64, CPython 3.14.7 and GCC 16.2.1:

| Check | Result |
|---|---|
| `python3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode full` | **PASS**, all 27 tests in 17.859 seconds, no skips; includes `test_generator_templates_seeded_domain_and_cpp` and both edited candidate fixtures |
| Saved pre-edit generator versus final generator, both compiled with `g++ -std=gnu++20 -O2` | **PASS**, byte-identical stdout/stderr and exit codes for seeds `0,1,42,2^63-1,2^63,2^64-1` and seven invalid-argument cases: missing, empty, negative, overflow, trailing junk, decimal fraction and extra argument |

P001/SUP01 remains complete. The existing linked inventory and completed checklist
entry continue to apply; quick mode alone was not used because it omits the
generator fixture.

## Post-migration defaults review (2026-09-27)

Reread the current [C++ guidelines](../00-Guidelines/03-cpp.md),
[maintenance workflow](../01-prompts.md#updating-a-completed-package-after-a-rule-change),
SUP01 row, inventories and [Core migration record](../00-Guidelines/22-core-migration.md).
The generator and all three C++ support snippets already comply; independent
review found no remaining discrepancy requiring a source edit.

- **Integers:** the standalone generator defines only `ulng = uint64_t` and keeps
  its complete unsigned seed domain. Its bounded counts/indices remain `int`.
  Numeric fixtures retain exact local `lng = int64_t` aliases for signed 64-bit
  payloads. Underlying spellings in alias definitions are intentional.
- **Names and grouping:** each program contains only `main`. Parsing, generation
  and fixture input/output steps are grouped coherently; no helper or public name
  requires renaming. Python conventions and protocol names remain unchanged.
- **Namespaces:** there are no owned C++ namespace definitions, shared helpers,
  public type families or wrapper headers to reorganize. `using namespace std`
  in the standalone examples does not export a detail namespace. Standard names
  outside the guideline's convenience list remain explicitly qualified, including
  `std::from_chars`, `std::errc` and `std::mt19937_64`.
- **Migration:** P001 has no Core includes or retired Core paths. The completed
  full/mini modular-family consolidation and Core renumbering need no P001 edits.
  No implementation, legacy original, generated file, shared checklist, index or
  scheduling map was changed in this review; only this evidence was updated.

Fresh verification: Linux x86_64, CPython 3.14.7, GCC 16.2.1, optimized GNU++20
fixture builds. The following command passed **all three selected tests in
4.092 seconds, with no skips**, including the generator test omitted by quick mode:

```bash
python3 - <<'PY'
import runpy, unittest
fixtures = runpy.run_path('96-Local Testing/00-Tools/01-contest_tester.py')
names = [
    'test_generator_templates_seeded_domain_and_cpp',
    'test_batch_pass_exact_and_cpp',
    'test_cpp_compile_once_flags_executables_and_failures',
]
suite = unittest.TestSuite(fixtures['ContestTools'](name) for name in names)
result = unittest.TextTestRunner(verbosity=2).run(suite)
raise SystemExit(not result.wasSuccessful())
PY
```

These checks cover deterministic generation through the maximum unsigned seed,
invalid seeds, output bounds, mixed Python/C++ execution, compile-once behavior,
custom compiler flags, existing executables and expected compilation failure.
The earlier full/stress, sanitizer and before/after comparison results above are
historical evidence; they were not rerun in this source-unchanged review. P001
remains complete with no new exception, failing reproducer or maintenance handoff.

## Reference review

Reviewed on 2026-09-27; these references informed protocol/lifecycle checks, with
no imported implementation code or added runtime dependency:

| Source | Selected material inspected and consequence |
|---|---|
| [Python subprocess documentation](https://docs.python.org/3/library/subprocess.html), Python 3.14.7, updated 2026-09-27 | `Popen`, `communicate`, timeout cleanup, pipe deadlock/memory warnings and `start_new_session`. `communicate` timeout does not itself kill the child; explicit cleanup is necessary. The tools use APIs available in Python 3.10. |
| [Mike Mirzayanov testlib.h](https://github.com/MikeMirzayanov/testlib/blob/master/testlib.h), version 0.9.45 | File-format introduction, `TResult`, `registerGen`, `registerInteraction`, `registerValidation`, finite-score checks. Confirmed the seed/input/checker/interactive/scoring families and the need to distinguish invalid output from tool failure. This repository deliberately uses its own smaller hook contracts. In particular testlib presentation-error exit 2 needs an adapter mapping to contestant failure; testlib interactors also take an additional output-file argument. Raw testlib hooks are not universally drop-in compatible. |

Retrieved snapshot SHA-256 values (for identifying this review, not runtime inputs):

- Python page: `4f11546e19df639ff09b1bd57f25346f3be6db8ebe70529ee636e931dbd05982`.
- testlib source: `bb323e3c89285214966076e0d23d5a295c5f6126da7ff198c1276ddb95ecb1a0`.

The [migration notes](../00-Guidelines/11-migration.md) identify these tools as new
support files from the reorganization, not transferred algorithm implementations.
Their prior fixture claims were treated as reference evidence and rerun, not as
proof of correctness. No legacy files were deleted or superseded here.

## Limits and follow-up ownership

- POSIX process-group semantics are required. Programs deliberately starting new
  sessions can escape cleanup. Output/resource quotas, hostile-code isolation,
  Windows process supervision and saving interrupted partially judged cases are
  outside this short local toolkit's contract. Batch output occupies memory;
  interactive transcripts occupy disk proportional to observed traffic.
- Templates and hooks remain problem-specific. A generator enforces its input
  domain; the batch runner has no separate input-validator phase. Nonunique
  witness checkers and numerical tolerance policies must be supplied explicitly.
  There is no implicit conversion of exact integer answers to floating point.
- New algorithm packages own their graph/tree/multigraph, numeric-boundary,
  string and legal-history generators/checkers. They also own structure-preserving
  shrink hooks and interactive/scored probes when their problem demands them.
  Arbitrary semantic shrink metrics, a separate global shrink timeout and generic
  reducers remain conditional planned extensions in the folder index. Nothing in
  P001 claims those future adapters are implemented.
- CPython and GNU++20 were exercised on Linux; exact Python 3.10, PyPy and macOS
  execution are not available here. Syntax is checked against Python 3.10. No
  algorithm performance claim or optimization threshold is introduced, so a
  throughput benchmark is not a completion requirement for these fixes.

Handoff: no failing reproducer or unfinished SUP01 feature remains. Conditional
future adapters stay **planned** in the folder index, owned by the algorithm or
problem package that motivates them. Start such work from its input domain and
checker/probe contract, add independent fixtures here as appropriate, and retain
the shared runner's existing protocol regressions.
