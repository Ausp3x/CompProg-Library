---
paths:
  - "97-Online Testing/**"
  - "99-Workspace/**"
---

# Online testing, expansion, workspace

97-Online Testing owns judge-specific source solutions under src/<judge>/ and generated outputs under expanded/<judge>/. Judge/topic directories may be numbered for sorting. Each source records judge/problem URL or ID and which features/variants it exercises. Add CSES, Yosupo, AtCoder, Codeforces, AOJ, SPOJ, Kattis or other suitable judges as needed. Do not auto-submit or claim acceptance without evidence. Distinguish local compilation/sample checks from accepted online runs.

The expander runs from any working directory, discovers unexpanded C++ sources recursively under src only, and maps `name.cpp` to `name.expanded.cpp` preserving relative directories. Use oj-bundle for actual C++ preprocessing/bundling behavior; report how to install it when unavailable. Never consume expanded outputs as inputs. Do not modify original sources. Output replacement is atomic on successful expansion; failed expansion preserves the prior valid output and exits nonzero.

Cleanup is ON by default. `--noclean` retains obsolete generated outputs; `--clean` explicitly selects the default. Delete only owned generated outputs whose source no longer exists, using a manifest, never arbitrary files or sources. If discovery/bundling fails, preserve prior outputs and do not perform destructive cleanup. Generated files/metadata/build artifacts are excluded from version control. Keep judge source code canonical; generated files are reproducible derivatives.

Python online programs follow the same source/output separation if a Python bundler is introduced. Preserve import/dependency semantics and test expansion; current C++ bundling must not pretend to support Python. Native standalone Python submissions may remain under src.

99-Workspace has one permanent file: template.cpp. It is a standalone generated snapshot of Core template.hpp + debug.hpp, opening with the motto line, with local includes, pragma-once and doc-comment lines removed, followed by a multi-case driver: solve(int t) and main() with ios::sync_with_stdio(false); cin.tie(nullptr); int t = 1; cin >> t; and solve(i) for i = 1..t. Keep generation tooling elsewhere; regeneration is an explicit command, never a side effect of tests/builds/expansion. Temporary contest files are user work, not automatic cleanup targets. No extra permanent helper/config files belong in Workspace.
