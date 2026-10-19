# P003 / SUP05 workspace verification

P003 owns the explicit refresh and verification of `99-Workspace/template.cpp`; its prerequisite P002 is
verified ([C01 evidence](../../01-Core/Docs/02-debug.md)).

## Scope and contracts

The snapshot follows [06-online.md](../../00-Guidelines/06-online.md): it opens with
the motto line `// 知彼知己，百战不殆`, then contains the Core template and debug
headers in that order with `#pragma once`, Debug's local include of the template
and every other line starting with `//` removed, followed by the driver
`void solve(int t)` and `main()` with `ios::sync_with_stdio(false); cin.tie(nullptr);
int t = 1; cin >> t;` and `solve(i)` for `i = 1..t`. It needs no repository
include path. Its GNU C++20, GCC 14+ contract and template/debug API domains
are those of C01.

Driver domain: valid contest input whose first token is a nonnegative `int` case
count. An empty input fails the stream sentry, leaves `t = 1` and calls `solve(1)`
once; this is intended and tested. The untouched stub does not use `t`, so
`-Wextra` reports one `-Wunused-parameter` warning; the signature is fixed by the
guideline and the stub is user work, so the warning is accepted and is the only
one allowed in the matrix below.

The driver layout (`}` on its own line after `return;`, the `for` body and
`return 0;`) is the user's canonical template ([decision log](../../00-Guidelines/10-decisions.md),
Workspace template) and is exempt from the `03-cpp.md` closing-brace rule;
`03-consistency.py --braces 99-Workspace/template.cpp` reports exactly these three
lines. The Core headers it is built from remain subject to the rule through P002.

Generation (`97-Online Testing/03-workspace.py`) is explicit and script-relative,
with atomic replacement; the written file gets mode `0666 & ~umask`. Local
include and `#pragma once` lines are recognised with an optional trailing comment.
`--check` compares without writing and exits nonzero
with a diff when the snapshot is missing or stale. `99-Workspace` holds only
`template.cpp`; temporary contest files are never cleanup targets.

## Feature-to-check map

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

## Commands and results (2026-10-06)

CPython 3.14, GCC 16.2.1 (`20260810`), Linux x86-64, invoked from `/tmp`:

```bash
python3 '/home/Ausp3x/Documents/CompProg Library/97-Online Testing/03-workspace.py' --check     # Current
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/00-Tools/02-online_tester.py' # 4 PASS
python3 '/home/Ausp3x/Documents/CompProg Library/97-Online Testing/03-workspace.py'             # Generated (identical bytes, mode 0644)
python3 /tmp/p003-workspace-smoke.py                                                          # PASS: 12 builds, 42 runs; empty-input PASS
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/02-integration.py'           # PASS: 102 headers, scalar/AVX2 multi-TU, workspace (run before the review fixes; snapshot bytes unchanged since)
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/03-consistency.py'           # errors: []
```

The smoke script (a one-time `/tmp` artifact, not a permanent suite) copies the
snapshot to a temporary directory (path with spaces) and builds the untouched
and probe copies with `-std=gnu++20 -Wall -Wextra -Werror` under
`-O2 -DNDEBUG`, `-O0 -g -D_GLIBCXX_ASSERTIONS` and
`-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all`, each with and
without `-DLOCAL` (12 builds); only the untouched copy gets
`-Wno-error=unused-parameter`. Untouched copies run on inputs empty, `0`, `1 -7`
and `3 -7 0 9223372036854775806` with empty stdout/stderr; probes run the three
nonempty inputs with exact output. Deterministic inputs, no seed; timeouts 180 s
compile, 10 s run. The snapshot's SHA256 is checked unchanged afterwards. The
retained regressions are `--check`, the fixture and integration.

No algorithm, ISA path or performance claim is involved, so no benchmark applies.

## Provenance

Sources inspected: the generator, the Core template and debug headers with their
C01 evidence, [06-online.md](../../00-Guidelines/06-online.md), the
[Core migration record](../../00-Guidelines/History/2026-09-27-core-migration.md) and
`OLD/Workspace/template.cpp`. No external code or new algorithm was adopted.
`OLD/Workspace/template.cpp`'s multi-case `solve(int)` wrapper is the current
design; its template/debug features are accounted for by C01.

## History

- 2026-09-27: P003 completed with the single-case driver; `--check`, the online-tool fixture and a 12-build, 20-run smoke matrix passed on CPython 3.14.7 and GCC 16.2.1.
- 2026-09-27: Core namespace migration regenerated the snapshot; integration now fails clearly on a missing snapshot, resolving the SUP02 reporting handoff.
- 2026-09-27: post-migration maintenance review, no source edit; `--check`, the fixture, the smoke matrix and consistency passed.
- 2026-10-06: re-audit for the multi-case driver; stale fixture and four review findings fixed; `--check`, the fixture, a 12-build, 42-run smoke matrix, integration and consistency passed.
