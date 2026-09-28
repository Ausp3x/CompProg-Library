# Library verification and performance evidence

96-Local Testing verifies reusable library files; 09-Contest Testing compares problem solutions. Do not confuse their purposes or present online problem acceptance as complete feature coverage.

## Organization and execution

Mirror algorithm paths: `96-Local Testing/01-Core/05-modint_tester.py`, supporting `.cpp` and auxiliary checks alongside it. One runnable Python entry per header/module, not necessarily one physical test file. Shared infrastructure is encouraged; add a run-all entry with filtering. Tests include the actual header and run from any working directory. Compile outputs go to temporary/build paths, never among canonical headers or Workspace.

Expose quick/full/stress modes with their actual coverage documented. Quick is a smoke/regression subset, full covers every public feature and required configuration, stress adds extended exhaustive/random/adversarial workloads. Do not label a truncated test run full. Existing migrated suites may have narrower interfaces; record the gap rather than pretending unsupported modes work. External test-only dependencies (pytest, Hypothesis, independent numeric libraries) are allowed and encouraged when useful; provide reproducible setup and clear missing-dependency messages.

After changing filenames, inventory ownership or package scheduling, run `python3 '96-Local Testing/03-consistency.py'`. The shared runner includes this check with integration; `--no-integration` skips both. Keep historical source keys, archived bytes and recorded benchmark hashes distinct from current destinations. A passing consistency check establishes agreement between records, not algorithm correctness.

Named tests, in-place progress on terminals (``), readable line output when redirected, green PASS/red FAIL when color is supported, nonzero exit on failure, explicit SKIP for unavailable optional hardware/interpreters. Always report seed, smallest known reproducer, operation/input, expected/actual values, configuration, command and timeout/crash information. Failures must survive -DNDEBUG; do not use removable assert as the only test oracle. Never silently ignore subprocess failures.

## Coverage requirements

Maintain a per-file feature-to-test map. Cover public functions/operators, type/domain combinations, setup/reset/lifetime/state transitions, copy/move/aliasing where relevant, valid no-answer results, and declared precondition failures. Cover empty/singleton, boundaries, extreme magnitudes, zero/negative/sign cases, duplicate/degenerate inputs, disconnected/self-loop/multiedge graphs where allowed, thresholds/alignment boundaries, adversarial shapes and repeated calls/cache changes.

Combine independent brute-force/reference oracles, bounded exhaustive small domains, algebraic/metamorphic invariants, deterministic random stress and saved regressions. Shared bugs make full-vs-mini or C++-vs-Python agreement insufficient as sole evidence. Validate reference code as carefully as test generation. Approximate comparisons use justified absolute/relative/error bounds; do not convert exact large integers to float.

Use optimized GNU++20 builds, checked/assertion builds, and AddressSanitizer/UndefinedBehaviorSanitizer where applicable. Test -DNDEBUG and LOCAL/non-LOCAL when behavior differs. Test scalar and each specialized compiled path on compatible hardware; skip unsupported ISA execution explicitly. Test full/mini shared-domain equivalence and reduced-domain preconditions. Compile each header alone, Basic/All aggregates, mixed full/mini use and multiple translation units. Test both CPython and PyPy when available. Prioritize relevant checks for the change; broaden only for unresolved concerns/new failures.

No finite suite proves all inputs. Completion means every specified feature and known edge-case class has justified coverage, remaining limits are disclosed, and the implementation has a correctness argument. Avoid tests merely duplicating implementation logic.

## Benchmarks

Separate timing from correctness. Record CPU, compiler/version/flags, interpreter, input distributions/sizes, seeds, warmup/repetitions, setup cost, memory, and both implementations being compared. Use repeated measurements/medians and verify outputs; optimization cannot remove the benchmarked work. Cover small/common/large/adversarial inputs and dispatch boundaries. Compilation time is irrelevant to optimization choices.

Apply principles' >=20% end-to-end reduction rule for Barrett/Montgomery outside Core; record any justified exception and common-case regressions. Distinguish reproducible performance evidence from noisy shared-machine observations. Avoid universal hard timing gates across different hosts.
