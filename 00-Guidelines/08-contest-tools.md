---
paths:
  - "09-Contest Testing/**"
---

# In-Contest Problem Testing

`09-Contest Testing` is a short editable toolkit, not the library's test framework in `96`. It orchestrates with the Python 3.10 standard library, compiles GNU++20, uses ordinary temporary files and needs no privilege changes. Generators, checkers, interactors and scorers are small separate templates. Cases never run in parallel.

Batch pipeline: `generator(seed)` produces one complete input, the candidate and an optional trusted brute force read that exact input, and their outputs are compared. Programs may be Python, C++ compiled once per invocation, or existing executables. The default comparison is token-exact and ignores only whitespace; a byte-exact mode exists. A custom checker receives the input, actual and reference paths, for nonunique answers or explicit floating-point tolerances. Large integer tokens are never silently compared as floating point.

A run continues until a failure or an interruption by default; a case count, start seed, timeout, compiler and interpreter are optional settings. Timeouts apply to the generator, candidate, reference and checker. A candidate's wrong answer, crash or timeout is reported apart from a failed generator, reference or checker, which is inconclusive. Nothing depends on the caller's working directory or on shebangs, and stderr stays separate from the answer output.

A failure is saved as a uniquely named bundle that never overwrites an earlier one: the input, both outputs, stderr, seed, commands and flags, comparison mode, exit or timeout status and the relevant configuration. Replay uses the saved input, not only a regenerated seed. Progress output is compact and results explicit.

Interactive mode pairs the solution with an interactor, enforces total and idle (deadlock) timeouts, closes pipes and child processes correctly, and saves the transcript of both directions with the hidden case, seed and commands. The hidden case never reaches the solution. An interactor error is distinct from a rejected answer, with defined exit codes, and replay documents its dependence on a deterministic interactor.

Scored mode validates outputs and computes finite numeric scores under a declared maximize or minimize objective. An invalid answer is a correctness failure; different valid scores are quality differences, not wrong answers. A reference score is optional, and any stop-on-regression policy is documented.

Extraction (`10-grab.py`, user decisions 2026-11-05) is the one tool here beyond the standard library. It needs libclang (`TOOLS` in `/setup-check`), optional: without it grab prints SKIP and inlines whole headers. It copies library lines verbatim, never rewrites them, and leaves out the template and debug headers, which the contest template holds. It parses each header in a scalar and an ISA configuration and caches the index in the git-ignored `09-Contest Testing/.grab-cache`, never in `99-Workspace`. What a name keeps and where `--into` puts its block: `96-Local Testing/Docs/00-extraction.md` (Contracts); checks: `05-testing.md`, Extraction. `--find` (user decision 2026-11-06) ranks the inventory rows from the folders' `00-index.md` and the evidence documents' aliases alone, with the standard library, OpenCC's Chinese tables (`_11_search_t2s.tsv`, THIRD-PARTY.md) and no index (`_11_search.py`); its contracts, weights and evaluation corpus: `96-Local Testing/Docs/00-search.md`.

Shrinking is an optional separate helper, off by default. A problem-specific hook proposes smaller inputs; only valid ones that reproduce the original failure class and signature are kept. Attempts and time are bounded, the original and reduced cases recorded, and the size metric strictly decreasing and well-founded. Arbitrary text deletion is not a valid general shrinker. Interactive and scored shrinking need problem-specific reproduction, never the batch equality reused blindly.
