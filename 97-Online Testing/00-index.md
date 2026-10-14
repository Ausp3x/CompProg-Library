# Online testing

`src/` holds canonical judge solutions under `src/<Judge>/<Topic>/` (Title Case directories). The `expanded/` outputs and the `02-expansion.json` manifest are git-ignored artifacts of `01-expander.py`; none exist until the expander runs, which needs `oj-bundle`. The current set has **20 Yosupo C++ sources**: 11 linear algebra and 9 big integer programs. They were migrated from the earlier library layout; online acceptance has not been verified. Source comments identify their problems.

Run the C++ expander from any directory:

```bash
python3 "/path/to/library/97-Online Testing/01-expander.py"
```

It recursively bundles unexpanded `src/**/*.cpp` with `oj-bundle` (files ending `.expanded.cpp` are excluded even if misplaced under src), preserving relative directories and writing `name.expanded.cpp` under `expanded/`. The per-file bundler timeout defaults to 120 seconds (`--timeout` changes it). Install the bundler with `python -m pip install online-judge-verify-helper`, or pass its executable with `--bundler /path/to/oj-bundle`. If bundling fails, the command exits nonzero and keeps prior outputs. Cleanup of orphaned manifest-owned outputs is on by default (`--clean`); use `--noclean` to retain them. Modified generated files are protected from overwrite or deletion. Python sources may remain standalone under `src/`; this tool reports them but supports C++ expansion only.

`03-workspace.py` regenerates the standalone `99-Workspace/template.cpp` snapshot from `01-Core/01-template.hpp` and `02-debug.hpp`; after template or debug changes, `--check` displays a diff and exits nonzero if the snapshot is stale. Snapshot contents and the regeneration rule: [06-online.md](../00-Guidelines/06-online.md). P003/SUP05 evidence (2026-10-06 re-audit): [workspace verification](Docs/00-workspace-verification.md).

The [Core family migration](../00-Guidelines/History/2026-09-27-core-migration.md) (2026-09-27) updated the sources' Core includes, expanded all 20 with the real bundler and refreshed Workspace after the namespace comment change. That record distinguishes its fresh local checks from the earlier P003 evidence.

## Judge coverage backlog

The inventory audit maps Library Checker problem families to planned canonical targets; it does **not** add online solutions or acceptance records. The 20 migrated Yosupo sources above remain the actual current set. After a target's API and local tests stabilize, add the relevant small judge wrapper and record the feature it exercises:

| Planned source group | Representative coverage |
|---|---|
| Yosupo data structures/trees | Noncommutative weighted DSU and folds, persistent/dynamic operations, range sorting/mode/LIS, multidimensional updates, tree contours and dynamic tree DP. |
| Yosupo graphs | Circulation with demands, weighted general matching, ranked walks, three-edge components, chordal recognition, minimum-diameter trees and restricted dynamic cuts. |
| Yosupo arithmetic/strings/geometry | Exact and modular algebra, FPS/transforms, suffix/palindrome/run families and available exact geometric tasks. Consult the mapped problem catalog rather than assuming every operation has a judge. |
| CSES, AtCoder, AOJ, Codeforces, Luogu, SPOJ, Kattis and other suitable judges | Complementary domains and practical limits; constructive/numerical validators and problem-specific interactive/scored adapters where applicable. These are future source groups, not existing directories or accepted submissions. |
| Python subset | Standalone judge programs for deliberately selected Python algorithms; a Python dependency bundler remains unimplemented. |

Record each wrapper as **planned**, **locally compiled/run**, **sample checked** or **accepted online** ([06-online.md](../00-Guidelines/06-online.md)); an acceptance record needs a judge/problem link, submission evidence, language/compiler, date and the source revision or hash tested. The repository has no CI.
