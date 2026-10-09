---
paths:
  - "96-Local Testing/**"
---

# Library tests and benchmarks

`96-Local Testing` verifies library files. `09-Contest Testing` compares problem solutions. Judge acceptance is never library coverage evidence.

## Layout and modes

- Mirror the library path: `96-Local Testing/02-Data Structures/01-dsu_tester.py` with helper `.cpp` files beside it. One Python entry per header or module; shared infrastructure in `_00_cpp_test_runner.py`. Tests include the real header and run from any working directory; build outputs go to a temporary directory.
- Modes: `quick` (smoke subset, seconds), `full` (every operation and configuration), `stress` (extended random, exhaustive and adversarial, seeded, with `--rounds`). Document the actual coverage of each mode; never label a truncated run `full`.
- Register the entry in `01-run.py` discovery. After any rename, inventory or plan change run `python3 '96-Local Testing/03-consistency.py'`.
- `03-consistency.py` also enforces the `03-cpp.md` closing-brace rule on every header whose inventory row and owning package are both `verified`, and on that header's C++ testers and benchmarks under `96-Local Testing`. It ignores comments, literals and preprocessor lines and exempts initializer braces; `python3 '96-Local Testing/03-consistency.py' --braces <files>` checks arbitrary files for the brace rule, the comment cap and, for `01-Core` and `02-Data Structures` headers, the type-header rule (no public free function outside a struct, its `<Type>Companion` or `<family>_detail`; enforced on verified type headers as `03-cpp.md` defines them) while editing. A re-audit normalizes the package's files before its package is marked verified.
- Output: named tests, in-place progress on a terminal, plain lines when redirected, green PASS and red FAIL, nonzero exit on failure, explicit SKIP for missing hardware or interpreters. A failure prints seed, smallest known reproducer, operation and input, expected and actual values, configuration, command, and timeout or crash details. Failures must survive `-DNDEBUG`; never use a removable `assert` as the only oracle. Never ignore a subprocess failure.

## Memory safety

- Every tester, benchmark driver and `02-integration.py` calls `_00_memory_cap.ensure()` on start; the shared folder runners do it on import, and `03-consistency.py` rejects an entry point that does not. The entry point then runs inside a transient systemd user scope: `MemoryMax` is the cap (default 4096 MB, `CP_TEST_MEMORY_MB` overrides), swap is disabled, and `OOMPolicy=continue` kills only the runaway process, which the runner reports as a FAIL. A suite that legitimately needs more memory passes `ensure(mb=...)` and records the reason and its measured peak in its evidence.
- Full and stress runs first rerun the same entry point in quick mode under a 2048 MB pre-flight cap (`CP_TEST_PREFLIGHT_MB`). The long run starts only when quick passes; `CP_TEST_PREFLIGHT=0` disables the pre-flight for a deliberate rerun.
- Each capped run ends with `MEMORY peak=... cap=... oom_kills=...`. Evidence records the peak of the full and stress runs.
- Ad-hoc probes, scratch binaries and one-off benchmarks also run capped: `python3 '96-Local Testing/_00_memory_cap.py' [--mb N] -- <command>`. Never run a compiled test program uncapped.
- Oracle, stdin and random inputs given to an optimized (`-DNDEBUG`) build stay inside every documented domain. Precondition violations are exercised only by the checked build's `--invalid` probes.

## Coverage

- Keep a feature-to-test map in the evidence document: every operation in the inventory row maps to at least one test.
- Call every operation as user code does: `friend` functions unqualified through ADL, `static` builders as `Type<T>::name`, companion functions as `TypeCompanion::name`.
- Oracles in priority order: independent brute force, bounded exhaustive small domains, algebraic or metamorphic identities, deterministic seeded random stress, saved regressions. Agreement between full and mini or between C++ and Python is supporting evidence only, never the sole oracle. Validate the brute force itself.
- Cases: empty and singleton, boundaries of every documented domain, extreme magnitudes, zero and negative values, duplicates and degeneracies, disconnected graphs, self-loops and multi-edges where allowed, threshold and alignment neighbours, aliasing of inputs and outputs, repeated calls, reset and cache changes, valid no-answer inputs, and each documented precondition failure under a checked build.
- Builds: optimized GNU++20, checked (`-O2 -g -fsanitize=address,undefined`), `-DNDEBUG`, `LOCAL` and non-`LOCAL` when behavior differs; header alone, `98-basic`, `99-all`, mixed full and mini, two translation units; scalar and each ISA path with an explicit SKIP where the CPU lacks it; CPython and PyPy for Python. Use `-Wall -Wextra -Wconversion`; a warning is acceptable only when the bound is stated in the code.
- Approximate comparisons use justified absolute or relative tolerances; never compare exact big integers through floats.

## Benchmarks

- Separate from correctness. Record CPU, compiler and flags, interpreter, input sizes and distributions, seeds, warmup and repetition counts, medians, memory, and the compared implementation. Verify outputs inside the benchmark so the work cannot be optimized away.
- Drivers are `<NN-name>_benchmark.py/.cpp` beside the tester (`00-<stem>_benchmark.*` for a folder-wide driver). Raw logs `*_benchmark.json`/`.jsonl` are written beside the driver and are ignored by git; commit only the summary table with conditions and medians in the header's evidence document.
- Cover small, typical, large and adversarial inputs and every dispatch threshold. The Barrett/Montgomery 20% rule from principles applies outside Core. No fixed timing gates across machines.
