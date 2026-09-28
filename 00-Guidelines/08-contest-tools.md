# In-contest problem testing

09-Contest Testing is a short editable toolkit, not the maintained library test framework in 96. Use Python3.10 standard-library orchestration, GNU++20 compilation, ordinary temporary files and no required privilege changes. Keep generators/checkers/interactors/scorers as small separate templates. No parallel case execution.

Batch pipeline: generator(seed) produces one complete input; candidate and optional trusted brute read that exact input; compare outputs. Support Python, C++ compiled once per invocation, and existing executables. Default token-exact comparison ignores whitespace only; offer byte-exact mode. Custom checker receives input/actual/reference paths, for nonunique answers or explicit floating-point tolerances. Do not silently treat large integer tokens as floating point.

Default run continues until failure/interruption; optional finite count/start seed/timeout/compiler/interpreter settings. Apply timeouts to generator, candidate, reference and checker. Differentiate candidate wrong answer/crash/timeout from a failed generator/reference/checker (inconclusive infrastructure/reference failure). No I/O protocol dependence on caller cwd or executable shebang availability. Keep stderr separate from answer output.

On failure, preserve a uniquely named artifact bundle: actual input, both outputs, stderr, seed, commands/flags, comparison mode, exit/timeout status and relevant configuration. Do not overwrite previous failures. Replay saved input rather than relying only on regenerating a seed. Progress is compact and results explicit.

Interactive mode pairs solution with an interactor, enforces total and idle/deadlock timeouts, closes pipes/children correctly, and saves the bidirectional transcript plus hidden case, seed and commands. Hidden case input must not accidentally be sent to the solution. An interactor error is distinct from a rejected answer; define exit codes. Replay must document dependence on deterministic interactor behavior.

Scored mode validates outputs and obtains finite numeric scores under a declared maximize/minimize objective. Invalid answers are correctness failures; different valid scores are quality differences, not automatically wrong answers. A reference score is optional; document any stop-on-regression policy explicitly.

Optional shrinking lives in a separate helper and is disabled by default. A problem-specific hook proposes smaller inputs; validate them and retain only those reproducing the original failure class/signature. Bound attempts/time, record the original and reduced cases, and require a strictly decreasing well-founded size metric. Do not claim arbitrary text deletion is a valid general shrinker. Interactive/scored shrinking requires suitable problem-specific reproduction, not blindly reusing batch equality.
