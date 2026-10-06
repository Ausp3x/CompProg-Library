# Contest testing notes

Contracts live in [00-index.md](00-index.md); evidence in [10-verification.md](10-verification.md).

## Completeness decisions (P001 re-audit, 2026-10-06)

Added after the reference sweep in [81-sources.md](81-sources.md): `--input-validator` (testlib `registerValidation`, Kattis input validators) and `--replay ... --expected` answer-file comparison (oj, cf-tool, Kattis samples).

Left out on purpose:

- Built-in floating-point tolerance (`oj -e`, Kattis `float_tolerance`): the folder rule assigns tolerances to an explicit custom checker so exact integer tokens can never be compared as floats.
- Memory limits and peak-memory reports (`oj --mle`, Kattis limits): resource quotas are outside this local toolkit's contract, `RLIMIT_AS` breaks ASan builds and `sys/resource` limits are not judge-faithful.
- Generator argument passthrough (testlib `opt<>`): generators are editable templates; a seed-only argv keeps replay metadata exact.
- Parallel jobs: forbidden by the folder rule. Sample download/submission: never submit. Raw testlib/Kattis exit-code conventions, interactor-to-checker output files and test groups/points: adapter or package work, too heavy for the short toolkit. Diff display and line-sensitive comparison: presentation, or a custom checker. Keep-going after failure and duplicate-input detection: present in only one surveyed tool.
