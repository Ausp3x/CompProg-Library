# P003 / SUP05 Workspace Verification

P003 owns the explicit refresh and verification of `99-Workspace/template.cpp`. Its prerequisite P002 is verified ([C01 evidence](../../01-Core/Docs/02-debug.md)).

## Scope and Contracts

- Snapshot ([06-online.md](../../00-Guidelines/06-online.md)): the file opens with the motto line `// 知彼知己，百战不殆`, then holds the Core template and debug headers in that order. `#pragma once`, Debug's local include of the template and every other line starting with `//` are removed. The driver follows: `void solve(int t)` and `main()` with `ios::sync_with_stdio(false); cin.tie(nullptr); int t = 1; cin >> t;` and `solve(i)` for `i = 1..t`. The file needs no repository include path. Its GNU C++20, GCC 14+ contract and its template and debug API domains are those of C01.
- Driver domain: valid contest input whose first token is a nonnegative `int` case count. An empty input fails the stream sentry, leaves `t = 1` and calls `solve(1)` once; this is intended and tested.
- Warning: the untouched stub does not use `t`, so `-Wextra` reports one `-Wunused-parameter` warning. The guideline fixes the signature and the stub is user work, so this warning is accepted; it is the only one allowed in the build matrix below.
- Layout: the driver's `}` on its own line after `return;`, after the `for` body and after `return 0;` is the user's canonical template ([decision log](../../00-Guidelines/10-decisions.md), Workspace template). It is exempt from the `03-cpp.md` closing-brace rule, and `03-consistency.py --braces 99-Workspace/template.cpp` reports exactly these three lines. The Core headers it is built from remain subject to the rule through P002.
- Generation: `97-Online Testing/03-workspace.py` runs only when invoked, resolves paths relative to itself and replaces the file atomically; the written file gets mode `0666 & ~umask`. Local include and `#pragma once` lines are recognised with an optional trailing comment. `--check` compares without writing and exits nonzero with a diff when the snapshot is missing or stale.
- `99-Workspace` holds only `template.cpp`; temporary contest files are never cleanup targets.

## Feature-to-Check Map

| Feature | Evidence |
|---|---|
| Current header contents, motto, comment/pragma/include stripping | Real `03-workspace.py --check`; fixture checks motto prefix, exact remaining `//` lines, no `#pragma once`/local include, trailing-comment include, one `solve(int t)`/`main()` |
| Explicit generation; missing/stale snapshot; nonmutating check; failure atomicity; file mode | `02-online_tester.py::test_workspace` in a temporary root with spaces, run from `/tmp`; unexpanded include leaves the snapshot byte-identical; mode equals `0666 & ~umask`; real snapshot hash unchanged by the smoke matrix |
| Multi-case driver | Probe copy reads one `lng` per case and prints `Case t: x+1`: counts 0, 1, 3, values `-7`, `0`, `9223372036854775806` (reaches `INT64_MAX`), exact stdout |
| Empty-input default | Probe prints `solve t`: empty input gives exactly `solve 1` |
| Fast I/O, inherited helpers | Covered by C01 template tests; snapshot compiles and runs the probe through `cin`/`cout` |
| LOCAL behaviour | Probe under `-DLOCAL`: one `>>`/`<<` trace pair and one `debug` line per case on stderr, stdout unchanged |
| Non-LOCAL behaviour | Same probe without `LOCAL`: empty stderr |
| Integration | `02-integration.py` compiles the snapshot `-fsyntax-only` with and without `LOCAL`; fails clearly if the snapshot is missing |

## Commands and Results

Latest runs: `03-workspace.py --check` reported the snapshot current on 2026-11-22 ([05-modint.md](../../01-Core/Docs/05-modint.md), P005), and `02-online_tester.py` passed 5 checks on 2026-11-06/07 ([00-extraction.md](<../../96-Local Testing/Docs/00-extraction.md>)).

The block below is the 2026-10-06 re-audit, on CPython 3.14, GCC 16.2.1 (`20260810`) and Linux x86-64, invoked from `/tmp`:

```bash
python3 '/path/to/library/97-Online Testing/03-workspace.py' --check     # Current
python3 '/path/to/library/96-Local Testing/00-Tools/02-online_tester.py' # 4 PASS
python3 '/path/to/library/97-Online Testing/03-workspace.py'             # Generated (identical bytes, mode 0644)
python3 /tmp/p003-workspace-smoke.py                                                          # PASS: 12 builds, 42 runs; empty-input PASS
python3 '/path/to/library/96-Local Testing/02-integration.py'           # PASS: 102 headers, scalar/AVX2 multi-TU, workspace (run before the review fixes; snapshot bytes unchanged since)
python3 '/path/to/library/96-Local Testing/03-consistency.py'           # errors: []
```

The smoke script was a one-time `/tmp` artifact, not a permanent suite; the retained regressions are `--check`, the fixture and integration. It copied the snapshot to a temporary directory whose path has spaces and built the untouched and probe copies with `-std=gnu++20 -Wall -Wextra -Werror`, each with and without `-DLOCAL`, under three flag sets (12 builds):

- `-O2 -DNDEBUG`;
- `-O0 -g -D_GLIBCXX_ASSERTIONS`;
- `-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all`.

Only the untouched copy got `-Wno-error=unused-parameter`. Untouched copies ran on the inputs empty, `0`, `1 -7` and `3 -7 0 9223372036854775806` with empty stdout and stderr; probes ran the three nonempty inputs with exact output. Inputs were deterministic, with no seed; timeouts were 180 s per compile and 10 s per run. The snapshot's SHA256 was checked unchanged afterwards.

No algorithm, ISA path or performance claim is involved, so no benchmark applies.

## Provenance

Sources inspected: the generator, the Core template and debug headers with their C01 evidence, [06-online.md](../../00-Guidelines/06-online.md), the [Core migration record](../../00-Guidelines/History/2026-09-27-core-migration.md) and `OLD/Workspace/template.cpp`. No external code or new algorithm was adopted. The multi-case `solve(int)` wrapper of `OLD/Workspace/template.cpp` is the current design; C01 accounts for its template and debug features.

## History

- 2026-09-27: P003 completed with the single-case driver; `--check`, the online-tool fixture and a 12-build, 20-run smoke matrix passed on CPython 3.14.7 and GCC 16.2.1.
- 2026-09-27: Core namespace migration regenerated the snapshot; integration now fails clearly on a missing snapshot, resolving the SUP02 reporting handoff.
- 2026-09-27: post-migration maintenance review, no source edit; `--check`, the fixture, the smoke matrix and consistency passed.
- 2026-10-06: re-audit for the multi-case driver; stale fixture and four review findings fixed; `--check`, the fixture, a 12-build, 42-run smoke matrix, integration and consistency passed.
- 2026-11-22: P005 `/finish`: `--check` current.
