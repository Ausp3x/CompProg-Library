# P003 / SUP05 workspace verification

Completed 2026-09-27. P003 owns the explicit refresh and verification of
`99-Workspace/template.cpp`. Its prerequisite P002 is complete; see
[C01 evidence](../01-Core/21-c01-verification.md). The original completion records
below precede the Core namespace migration. The post-migration maintenance review
at the end records current hashes and fresh checks: the snapshot is current,
no further refresh was needed, and no owned gap remains.

## Scope and contracts

The snapshot contains the current Core template and debug headers, in that
order, with their `#pragma once` directives and Debug's duplicate local include
removed. It needs no repository headers when copied elsewhere. Its GNU C++20,
GCC14+, Linux x86-64 contract and template/debug API domains remain those of C01.

`main()` disables iostream synchronization, unties `cin`, and calls the empty
`solve()` once. The commented multi-case alternative now uses a braced loop and
explicitly says to replace the single call. It assumes valid contest input with
a nonnegative `int` test count. Executable setup remains outside Core.

Generation is explicit and script-relative. `--check` compares the expected
snapshot without writing it, returning nonzero with a diff for a missing or
stale file. Generation uses an atomic replacement after reading both headers.
`99-Workspace` still contains only its one permanent `template.cpp`; temporary
contest work is never a cleanup target. Tests generate only temporary fixtures.

The pre-refresh diff contained only P002's documented Debug layout changes;
there was no intervening solution code to preserve. The generator and its
existing fixture received the matching multi-case comment update. Core headers,
judge solutions, and legacy files were not changed.

## Feature-to-check map

| Feature | Evidence |
|---|---|
| Current header contents and standalone composition | Real `03-workspace.py --check`; existing workspace fixture checks directive removal and exactly one solve/main; isolated compilation without repository include paths |
| Explicit generation, missing/stale snapshots, nonmutating check | `00-Tools/02-online_tester.py::test_workspace`, from an unrelated working directory with paths containing spaces; real snapshot unchanged by runtime verification |
| Default entry point | Untouched snapshot exits successfully with empty stdout/stderr; temporary solve probe confirms one call even with a second input value present |
| Multi-case alternative | Temporary copy with the documented replacement runs zero, one and three cases, with exact output and no extra solve call |
| Fast I/O and inherited helpers | Probe checks disabled stream synchronization and null `cin` tie, exact alias identities, `chmin`/`chmax`, and PBDS pair-key rank/select |
| LOCAL behavior | Probe checks nested optional/variant formatting, one debug argument evaluation, and matched trace entry/exit |
| Non-LOCAL behavior | Probe passes nonexistent names to disabled debug/trace calls and confirms no evaluation or stderr |

## Original P003 commands and results

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
