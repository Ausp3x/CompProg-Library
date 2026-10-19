---
paths:
  - "96-Local Testing/**"
---

# Library tests and benchmarks

`96-Local Testing` verifies library files. `09-Contest Testing` compares problem solutions.

## Layout and modes

- Mirror the library path: `96-Local Testing/02-Data Structures/01-dsu_tester.py` with helper `.cpp` files beside it. One Python entry per header or module; shared infrastructure in `_00_cpp_test_runner.py`. Tests include the real header and run from any working directory; build outputs go to a temporary directory.
- Modes: `quick` (smoke subset, seconds), `full` (every operation and configuration), `stress` (extended random, exhaustive and adversarial, seeded, with `--rounds`). Document the actual coverage of each mode; never label a truncated run `full`.
- `01-run.py` discovers testers by glob; quick mode runs every folder except `01-Core`, whose quick testers are listed in `QUICK`. Run `03-consistency.py` after a rename or `plan.py set`, which bypass the post-edit hook.
- `03-consistency.py` enforces the `03-cpp.md` closing-brace rule and, on type headers, the type-header rule on headers whose row and owning package are both `verified`, and the brace rule on their C++ testers and benchmarks, ignoring comments, literals, preprocessor lines and initializer braces. While editing, `python3 '96-Local Testing/03-consistency.py' --braces <files>` checks any files for the brace rule, the comment cap and, on `01-Core` and `02-Data Structures` type headers, the type-header rule. In a linked worktree, ledger records of git-ignored artifacts (archive binaries and caches, benchmark logs) pass when the file is absent, so a fresh worktree validates without them; the main checkout keeps requiring them.
- Output: named tests, in-place progress on a terminal, plain lines when redirected, green PASS and red FAIL, nonzero exit on failure, explicit SKIP for missing hardware or interpreters. A failure prints seed, smallest known reproducer, operation and input, expected and actual values, configuration, command, and timeout or crash details. Failures must survive `-DNDEBUG`; never use a removable `assert` as the only oracle. Never ignore a subprocess failure.

## Memory safety

- Every tester, benchmark driver and `02-integration.py` calls `_00_memory_cap.ensure()` at start (the folder runners do it on import; `03-consistency.py` rejects an entry point that does not). It runs the entry point in a transient systemd user scope capped at 4096 MB by default (`CP_TEST_MEMORY_MB` overrides) with swap off, where only a runaway process is killed and the runner reports it as a FAIL. A suite that legitimately needs more memory passes `ensure(mb=...)` and records the reason and measured peak in its evidence.
- Full and stress runs first rerun the entry point in quick mode under a 2048 MB pre-flight cap (`CP_TEST_PREFLIGHT_MB`) and start only if it passes; `CP_TEST_PREFLIGHT=0` skips the pre-flight for a deliberate rerun.
- Each capped run ends with `MEMORY peak=... cap=... oom_kills=...`; evidence records the full and stress peaks.
- Ad-hoc probes, scratch binaries and one-off benchmarks run through `python3 '96-Local Testing/_00_memory_cap.py' [--mb N] -- <command>`; never run a compiled test program uncapped.
- Inputs given to an optimized (`-DNDEBUG`) build stay inside every documented domain; only the checked build's `--invalid` probes violate preconditions.

## Coverage

- Keep a feature-to-test map in the evidence document: every operation in the inventory row maps to at least one test.
- Call every operation as user code does: `friend` functions unqualified through ADL, `static` builders as `Type<T>::name`, companion functions as `TypeCompanion::name`.
- Before a wrapper or second implementation (`03-cpp.md`, One API per operation) is deleted, its test cases are reproduced against the kept operation and mapped in the feature-to-test map.
- Oracles in priority order: independent brute force, bounded exhaustive small domains, algebraic or metamorphic identities, deterministic seeded random stress, saved regressions. Agreement between full and mini or between C++ and Python is supporting evidence only, never the sole oracle. Validate the brute force itself.
- Cases: empty and singleton, boundaries of every documented domain, extreme magnitudes, zero and negative values, duplicates and degeneracies, disconnected graphs, self-loops and multi-edges where allowed, threshold and alignment neighbours, aliasing of inputs and outputs, repeated calls, reset and cache changes, valid no-answer inputs, and each documented precondition failure under a checked build.
- Builds: optimized GNU++20, checked (`-O2 -g -fsanitize=address,undefined`), `-DNDEBUG`, `LOCAL` and non-`LOCAL` when behavior differs; header alone, `98-basic`, `99-all`, mixed full and mini, two translation units; scalar and each ISA path with an explicit SKIP where the CPU lacks it; CPython and PyPy for Python. Use `-Wall -Wextra -Wconversion`; a warning is acceptable only when the bound is stated in the code.
- A package that changes a header also runs `python3 '96-Local Testing/02-integration.py'` before it is verified, adding `--sanitizers` when an `01-Core` header changed: the standalone, aggregate and multi-translation-unit builds of every header catch breakage in dependents that the header's own suite misses, and the sanitizer runs cover the Core testers (user decision 2026-10-19).
- Approximate comparisons use justified absolute or relative tolerances; never compare exact big integers through floats.

## Benchmarks

- Separate from correctness. Record CPU, compiler and flags, interpreter, input sizes and distributions, seeds, warmup and repetition counts, medians, memory, and the compared implementation. Verify outputs inside the benchmark so the work cannot be optimized away.
- Drivers are `<NN-name>_benchmark.py/.cpp` beside the tester (`00-<stem>_benchmark.*` for a folder-wide driver). Raw logs `*_benchmark.json`/`.jsonl` are written beside the driver and are ignored by git; commit only the summary table with conditions and medians in the header's evidence document.
- Cover small, typical, large and adversarial inputs and every dispatch threshold. The Barrett/Montgomery 20% rule from principles applies outside Core. No fixed timing gates across machines.
