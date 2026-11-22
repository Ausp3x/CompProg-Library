# Online Testing

Rules for sources, statuses, expansion, submission and the workspace: [06-online.md](../00-Guidelines/06-online.md).

`src/` holds the canonical judge solutions under `src/<Judge>/<Topic>/` (Title Case directories). The current set has **68 Yosupo C++ sources**: 11 linear algebra, 9 big integer, 46 polynomial and 2 number theory programs. The linear algebra and big integer programs were migrated from the earlier library layout; the polynomial and number theory programs were written and run locally against the regenerated judge tests ([judge sources](Docs/00-judge-sources.md)). Online acceptance is verified only where a source's Status line records it.

`01-expander.py` writes `expanded/` and its manifest `02-expansion.json`, both git-ignored, so neither exists until it runs:

```bash
python3 "/path/to/library/97-Online Testing/01-expander.py"
```

It bundles through [`10-grab.py`](<../09-Contest Testing/10-grab.py>); measured bundle sizes are in [00-extraction.md](<../96-Local Testing/Docs/00-extraction.md>). Without libclang grab inlines whole headers and says so. `--bundler oj-bundle` (or a path to it) runs `oj-bundle` with a per-file timeout of 120 seconds (`--timeout`); install it with `python -m pip install online-judge-verify-helper`. Generated files that were modified by hand are never overwritten or deleted. Python sources may stay standalone under `src/`; the expander reports them and bundles C++ only.

`04-submit.py` is the maintainer's submission tool: `python3 '97-Online Testing/04-submit.py' login` once, `list` for every Yosupo source with its state, and `python3 '97-Online Testing/04-submit.py' <selection>` to submit; with no selection it lists and asks. Evidence: [judge sources](Docs/00-judge-sources.md).

`03-workspace.py` regenerates `99-Workspace/template.cpp`; `--check` prints a diff and exits nonzero when the snapshot is stale. Evidence: [workspace verification](Docs/00-workspace-verification.md).

## Judge Coverage Backlog

[library-checker-coverage.json](../00-Guidelines/Ledgers/library-checker-coverage.json) maps Library Checker problem families to planned canonical targets; it adds no solutions or acceptance records. After a target's API and local tests stabilize, add the relevant small judge wrapper and record the feature it exercises:

| Planned source group | Representative coverage |
|---|---|
| Yosupo data structures/trees | Noncommutative weighted DSU and folds, persistent/dynamic operations, range sorting/mode/LIS, multidimensional updates, tree contours and dynamic tree DP. |
| Yosupo graphs | Circulation with demands, weighted general matching, ranked walks, three-edge components, chordal recognition, minimum-diameter trees and restricted dynamic cuts. |
| Yosupo arithmetic/strings/geometry | Exact and modular algebra, FPS/transforms, suffix/palindrome/run families and available exact geometric tasks. Consult the mapped problem catalog rather than assuming every operation has a judge. |
| CSES, AtCoder, AOJ, Codeforces, Luogu, SPOJ, Kattis and other suitable judges | Complementary domains and practical limits; constructive/numerical validators and problem-specific interactive/scored adapters where applicable. These are future source groups, not existing directories or accepted submissions. |
| Python subset | Standalone judge programs for deliberately selected Python algorithms; a Python dependency bundler remains unimplemented. |

CI runs no judge sources.
