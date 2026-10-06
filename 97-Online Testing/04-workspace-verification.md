# P003 / SUP05 workspace verification

Re-audited 2026-10-06 under the retooled system. P003 owns the explicit refresh
and verification of `99-Workspace/template.cpp`; its prerequisite P002 is
verified ([C01 evidence](../01-Core/21-c01-verification.md)). The sections after
"History" record the earlier single-case snapshot and are superseded where they
disagree with the current contract below.

## Scope and contracts

The snapshot follows [06-online.md](../00-Guidelines/06-online.md): it opens with
the motto line `// 知彼知己，百战不殆`, then contains the Core template and debug
headers in that order with `#pragma once`, Debug's local include of the template
and every other line starting with `//` removed, followed by the driver
`void solve(int t)` and `main()` with `ios::sync_with_stdio(false); cin.tie(nullptr);
int t = 1; cin >> t;` and `solve(i)` for `i = 1..t`. It needs no repository
include path. Its GNU C++20, GCC 14.2+ contract and template/debug API domains
are those of C01.

Driver domain: valid contest input whose first token is a nonnegative `int` case
count. An empty input fails the stream sentry, leaves `t = 1` and calls `solve(1)`
once; this is intended and tested. The untouched stub does not use `t`, so
`-Wextra` reports one `-Wunused-parameter` warning; the signature is fixed by the
guideline and the stub is user work, so the warning is accepted and is the only
one allowed in the matrix below.

The driver layout (`}` on its own line after `return;`, the `for` body and
`return 0;`) is the user's canonical template ([decision log](../00-Guidelines/10-decisions.md),
entry near line 206) and is exempt from the `03-cpp.md` closing-brace rule;
`03-consistency.py --braces 99-Workspace/template.cpp` reports exactly these three
lines. The Core headers it is built from remain subject to the rule through P002.

Generation (`97-Online Testing/03-workspace.py`) is explicit and script-relative,
with atomic replacement; the written file gets mode `0666 & ~umask`. Local
include and `#pragma once` lines are recognised with an optional trailing comment.
`--check` compares without writing and exits nonzero
with a diff when the snapshot is missing or stale. `99-Workspace` holds only
`template.cpp`; temporary contest files are never cleanup targets.

## Re-audit findings (2026-10-06)

- Pre-audit `--check` passed: the working-tree snapshot already matched the
  current template/debug and retooled generator. No refresh was needed.
- Defect fixed: the workspace fixture in `96-Local Testing/00-Tools/02-online_tester.py`
  still asserted the old single-case driver (`void solve()`, `std::ios::...`,
  commented `while (t--)`), so the suite failed. It also detected staleness by
  appending a `//` line, which the retooled generator now strips. The fixture now
  checks the motto prefix, exactly one `solve(int t)`/`main()`, the five driver
  statements, and uses a non-comment `FIXTURE_CHANGE` line for the stale/regenerate checks.
- `97-Online Testing/00-index.md` still described the single-case default; updated.
- No reaudit-findings file names P003.
- Independent review (2026-10-06) confirmed four low-severity findings, all fixed:
  1. `LOCAL_INCLUDE`/`PRAGMA_ONCE` required end of line, so `#include "01-template.hpp" // x`
     passed through and broke the snapshot; both now accept a trailing comment.
  2. `mkstemp` made every regenerated snapshot mode 0600; the temporary file is now
     chmodded to `0666 & ~umask` before `os.replace`. The real snapshot was explicitly
     regenerated (identical bytes) and is now `-rw-r--r--`.
  3. The driver's brace-rule exemption was unrecorded; see Scope and contracts.
  4. The fixture now asserts that only the motto and the driver's `// trace` line
     start with `//`, the umask-derived mode, a trailing-comment template include,
     and that an unexpanded include exits nonzero with the snapshot unchanged. The
     HEAD generator fails this fixture (mode check), confirming it detects the fix.
- Legacy: `OLD/Workspace/template.cpp`'s multi-case `solve(int)` wrapper, recorded below as
  superseded, is again the current design; its template/debug features remain accounted for by C01.
  The legacy file is unchanged.

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
python3 '/home/Ausp3x/Documents/CompProg Library/97-Online Testing/03-workspace.py' --check     # Current (before and after)
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/00-Tools/02-online_tester.py' # before fix: FAIL duplicate solve/main; after review fixes: 4 PASS
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
GCC 14.2 was not run (not installed); no online submission was made.

Current SHA256:

| File | SHA256 |
|---|---|
| `01-Core/01-template.hpp` | `23e6db3f682fd9f27657ace73f157a8631790d06c8c72984d9fcc102de7341e6` |
| `01-Core/02-debug.hpp` | `dcd9207d671ce45e12e04d92ffc76852300f27ec962ad74ab4e5bd6a9a5b6d83` |
| `97-Online Testing/03-workspace.py` | `0015e31088c58b3d82db90bc84a20d1e53a9929dcf75b873a37e143a23dcd4e3` |
| `99-Workspace/template.cpp` | `f6aa5003c6163e3fba441eb5508e21bc866cfe98faaccd37e1fc3ad59c8ea9df` |
| `96-Local Testing/00-Tools/02-online_tester.py` | `37ab6d5087c71ee655b72bfe4dbcd530ef71d9ef1d52c95c03b366cbfc3a5d16` |

## History: original P003 commands and results (2026-09-27, single-case driver)

Python 3.14.7 and GCC 16.2.1 (`20260810`), Linux x86-64. Commands were invoked
from `/tmp` using absolute repository paths, including the explicit refresh:

```bash
python3 '/home/Ausp3x/Documents/CompProg Library/97-Online Testing/03-workspace.py'
python3 '/home/Ausp3x/Documents/CompProg Library/97-Online Testing/03-workspace.py' --check
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/00-Tools/02-online_tester.py'
python3 /tmp/p003-workspace-smoke.py
```

All passed. The existing online-tool suite reported four PASS groups, including
the workspace fixture; its expander checks use a fake bundler and make no claim
about real judge expansion or acceptance.

The supplemental smoke script is a one-time local verification artifact in
`/tmp`, not a permanent suite or Workspace file. It compiled untouched,
single-case-probe and multi-case-probe copies under each of these configurations:

- `-std=gnu++20 -Wall -Wextra -Werror -O2 -DNDEBUG`, with and without `-DLOCAL`.
- `-std=gnu++20 -Wall -Wextra -Werror -O0 -g -D_GLIBCXX_ASSERTIONS`, with and without `-DLOCAL`.

All **12 builds and 20 executions** passed. Explicit checks survive NDEBUG.
Inputs were deterministic: single-case `7` followed by unused `100`; multi-case
counts 0, 1 and 3, including values `-7`, `0` and `9223372036854775806` whose
increment reaches the signed 64-bit maximum. There is no random seed. Compilation
and execution timeouts were 90 and 5 seconds respectively. All builds used
temporary paths and no repository include directories; the real snapshot's
bytes were checked unchanged afterward.

The permanent fixture and `--check` commands above are the retained regression
checks. To repeat the untouched-snapshot compile/run matrix independently:

```python
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path('/home/Ausp3x/Documents/CompProg Library')
with tempfile.TemporaryDirectory(prefix='p003-repeat-') as directory:
    build = Path(directory)
    source = build / 'template.cpp'
    shutil.copy2(root / '99-Workspace/template.cpp', source)
    for flags in (['-O2', '-DNDEBUG'], ['-O0', '-g', '-D_GLIBCXX_ASSERTIONS']):
        for local in ([], ['-DLOCAL']):
            binary = build / 'template'
            subprocess.run([os.environ.get('CXX', 'g++'), '-std=gnu++20',
                            '-Wall', '-Wextra', '-Werror', *flags, *local,
                            str(source), '-o', str(binary)],
                           cwd=build, check=True, timeout=90)
            result = subprocess.run([str(binary)], input='', text=True,
                                    capture_output=True, check=True, timeout=5)
            if result.stdout or result.stderr:
                raise RuntimeError(f'Unexpected output: {result}')
```

C01's full feature/sanitizer evidence remains in P002; those suites were not
rerun for this generated layout/comment update. No new algorithm, ISA path or
performance optimization was introduced, so benchmarking is inapplicable.
GCC14 itself was not run, and no online submission was made.

## Original provenance and completion hashes

The sources inspected were the current generator, Core headers and C01 evidence,
the workspace guideline, the relevant migration notes, and
`OLD/Workspace/template.cpp`. The legacy multi-case `solve(int)` wrapper has
already been superseded by the documented single-case default with optional
multi-case loop; its template/debug features are accounted for by C01. The legacy
file remains unchanged. No external code or new algorithm was adopted.

SHA256 at completion:

| File | SHA256 |
|---|---|
| `01-Core/01-template.hpp` | `251352f9cc65171c4527dc0596b40b21507652f0b2d0017c50431f67561964a2` |
| `01-Core/02-debug.hpp` | `cc29f0f453f7fdb698f24a643ec5e769102a0c333e1f15b3b920487845aefa8f` |
| `97-Online Testing/03-workspace.py` | `ac9fbcdb365e6fa45476e7df5798a964c4e0bdbf26a822f417985cc13d77de57` |
| `99-Workspace/template.cpp` | `2fede9e931ff09dbe1cfef0f98d86989baa03e415ffa043f748fedbbca5f90c6` |

## Original support handoff: SUP02 integration reporting (resolved)

The original read-only review found that `96-Local Testing/02-integration.py`
guarded Workspace compilation with `if workspace.exists()` but always included
"workspace" in its PASS message, silently skipping a missing snapshot. P003's
explicit `--check` and required-file isolated builds passed independently.

The requested support fix was a clear missing-file failure or honest skip, a
temporary-root reporting regression, and retained LOCAL/non-LOCAL compilation
without implicit generation. The Core migration implemented the failure and
regression; the fresh review below confirms this handoff remains resolved.

## Core namespace migration follow-up

The later [Core migration](../00-Guidelines/22-core-migration.md) added a namespace closing comment to Debug and explicitly regenerated the standalone Workspace snapshot. The completion hashes above identify the preceding P003 snapshot, not the later generated bytes. The original missing-workspace reporting gap is now resolved: integration fails clearly before compiling when the required snapshot is absent. `96-Local Testing/03-consistency.py` checks that failure using a temporary root, leaving the real snapshot unchanged. Migration-stage generation/check and build evidence belongs to the linked migration record.

## Post-migration maintenance review — 2026-09-27

Reviewed SUP05 against the current principles, C++ integer/namespace/style rules,
testing and Workspace guides, maintenance workflow, inventories, batch row and
Core migration record. The snapshot already matches its template/debug inputs,
including the four-space-indented public `Debug` namespace and separate labelled
closing line. No implementation, generator or test edit was needed. `--check`
passed before any generation; the real snapshot was not regenerated.

The inherited `size_t` uses match `std::bitset`/`std::index_sequence` templates and
the bounded `string_view` length conversion; `ptrdiff_t` retains its range-view
difference role. Exact width aliases, unsigned 128-bit magnitude arithmetic,
method grouping, public names and supported domains remain unchanged. The
single-case wrapper and braced multi-case alternative meet the current style.
Cohesive-family and mini-independence rules require no change to this template/
debug snapshot; the completed Core layout remains intact.

Fresh checks, invoked from `/tmp` with CPython 3.14.7 and GCC 16.2.1 (`20260810`):

```bash
python3 '/home/Ausp3x/Documents/CompProg Library/97-Online Testing/03-workspace.py' --check
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/00-Tools/02-online_tester.py'
python3 /tmp/p003-workspace-smoke.py
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/03-consistency.py'
```

All passed. The existing tool suite again passed four fixture groups. The
previously inspected supplemental smoke script was rerun against the **current**
snapshot: 12 isolated builds and 20 executions passed, using the same optimized
NDEBUG/checked, LOCAL/non-LOCAL configurations and deterministic cases documented
above. It checks untouched, single-case and multi-case copies, fast I/O, aliases,
helpers/PBDS, enabled debug/trace and disabled argument erasure. No repository
include path or real Workspace write is involved. The older results remain
historical; these are fresh runs rather than reused outcomes.

Consistency validation passed with no errors: 428 targets, 335 batches, 226
packages and 205 archived files. Its retained temporary-root regression requires
the missing-workspace exception. An independent temporary-copy CLI check also
returned exit 1, empty stdout and `FAIL: Required workspace snapshot missing:`
with explicit generation guidance. A nonexistent `CXX` demonstrated that this
failure occurs before compilation; no snapshot was created. The present snapshot
passed the separate build matrix above, so no reporting handoff remains open.

Only this evidence document changed in P003 maintenance. Template/debug,
Workspace, generation/integration tools, the existing online fixture and the Core
migration record retained their source hashes and modification times. The
completed checklist, current Core paths, other implementations and legacy
originals were preserved. Full Core feature/sanitizer suites and benchmarks were
not rerun for this review; their P002/migration evidence is separate. No new
performance, GCC14 execution or online acceptance claim is made.

Current SHA256 values, unchanged from the start of this review:

| File | SHA256 |
|---|---|
| `01-Core/01-template.hpp` | `251352f9cc65171c4527dc0596b40b21507652f0b2d0017c50431f67561964a2` |
| `01-Core/02-debug.hpp` | `2efc4730a89f75b079e5c394b9701cafb5dc38f7528fcd6f24df0a3795e8e08a` |
| `97-Online Testing/03-workspace.py` | `ac9fbcdb365e6fa45476e7df5798a964c4e0bdbf26a822f417985cc13d77de57` |
| `99-Workspace/template.cpp` | `11f9be9e307b483e7b1ee2bb2d47ddcf2a3a78356e84f7aa9726698128a29104` |
