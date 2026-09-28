# Online testing

`src/` holds canonical judge solutions. The current set has **20 Yosupo C++ sources**: 11 linear algebra and 9 big integer programs. They were migrated from the earlier library layout; online acceptance has not been verified in this migration. Source comments identify their problems. The generated `expanded/` tree and `02-expansion.json` are ignored build artifacts.

Run the C++ expander from any directory:

```bash
python3 "/path/to/library/97-Online Testing/01-expander.py"
```

It recursively bundles unexpanded `src/**/*.cpp` with `oj-bundle` (files ending `.expanded.cpp` are excluded even if misplaced under src), preserving relative directories and writing `name.expanded.cpp` under `expanded/`. The per-file bundler timeout defaults to 120 seconds (`--timeout` changes it). Install the bundler with `python -m pip install online-judge-verify-helper`, or pass its executable with `--bundler /path/to/oj-bundle`. If bundling fails, the command exits nonzero and keeps prior outputs. Cleanup of orphaned manifest-owned outputs is on by default (`--clean`); use `--noclean` to retain them. Modified generated files are protected from overwrite or deletion. Python sources may remain standalone under `src/`; this tool reports them but supports C++ expansion only.

`03-workspace.py` explicitly creates the standalone `99-Workspace/template.cpp` snapshot from `01-Core/01-template.hpp` and `02-debug.hpp`. Run it again only when you want to refresh the snapshot; `--check` displays a diff and exits nonzero if it is stale. Workspace generation does not run during expansion or tests.

**P003 / SUP05 verified (2026-09-27):** refreshed after P002's template/debug maintenance; `--check`, existing tool fixtures, and isolated LOCAL/non-LOCAL compile/run checks passed. The default remains single-case, with a braced commented multi-case replacement. See [feature coverage, commands, historical source hashes and the resolved SUP02 reporting handoff](04-workspace-verification.md).

The [Core family migration](../00-Guidelines/22-core-migration.md) updates the current Core includes, re-expands all 20 sources and explicitly refreshes Workspace after the namespace comment change. That record distinguishes fresh local checks from the earlier P003 evidence.

## Judge coverage backlog

The inventory audit maps Library Checker problem families to planned canonical targets; it does **not** add online solutions or acceptance records. The 20 migrated Yosupo sources above remain the actual current set. After a target's API and local tests stabilize, add the relevant small judge wrapper and record the feature it exercises:

| Planned source group | Representative coverage |
|---|---|
| Yosupo data structures/trees | Noncommutative weighted DSU and folds, persistent/dynamic operations, range sorting/mode/LIS, multidimensional updates, tree contours and dynamic tree DP. |
| Yosupo graphs | Circulation with demands, weighted general matching, ranked walks, three-edge components, chordal recognition, minimum-diameter trees and restricted dynamic cuts. |
| Yosupo arithmetic/strings/geometry | Exact and modular algebra, FPS/transforms, suffix/palindrome/run families and available exact geometric tasks. Consult the mapped problem catalog rather than assuming every operation has a judge. |
| CSES, AtCoder, AOJ, Codeforces, Luogu, SPOJ, Kattis and other suitable judges | Complementary domains and practical limits; constructive/numerical validators and problem-specific interactive/scored adapters where applicable. These are future source groups, not existing directories or accepted submissions. |
| Python subset | Standalone judge programs for deliberately selected Python algorithms; a Python dependency bundler remains unimplemented. |

For each wrapper distinguish **planned**, **locally compiled/run**, **sample checked**, and **accepted online**. Acceptance needs a judge/problem link, submission evidence, language/compiler, date and the source revision or hash tested. Keep local library tests authoritative for unsupported operations and boundary cases. No automatic submissions; expansion and CI compilation do not confer acceptance. Current repository CI is absent; any future online-source CI should compile/check samples and expansion without submitting.

`99-Workspace` was also reviewed: its only permanent file is the standalone `template.cpp`, containing template/debug plus fast iostream setup and the commented multi-case alternative. Do not add an index, tester or generator there. After template/debug changes, check staleness with `03-workspace.py --check`; refreshing the snapshot remains an explicit action and temporary contest files remain user work.
