---
paths:
  - "97-Online Testing/**"
  - "99-Workspace/**"
---

# Online Testing, Expansion, Workspace

`97-Online Testing` owns judge-specific source solutions under `src/<judge>/` and their generated outputs under `expanded/<judge>/`; judge and topic directories may be numbered for sorting. Each source records its judge and problem URL or ID and the features and variants it exercises. Add CSES, Yosupo, AtCoder, Codeforces, AOJ, SPOJ, Kattis or other suitable judges as needed. Each source's status is **planned**, **locally compiled/run**, **sample checked** or **accepted online**; an acceptance record needs the problem link, submission evidence, language and compiler, date and the source revision or hash tested.

Claude never submits (`01-principles.md`, Scope Discipline); `.claude/hooks/shell.py` refuses its runs of `04-submit.py`. The maintainer submits Yosupo sources with `04-submit.py` (user decision 2026-11-15): `login` once (the password is never stored; a refresh token stays outside the repository), then a selection by list number, problem, file name, path, glob or `new`/`stale`/`all`. It expands first, submits today's bundle as the logged-in user, waits for each verdict and writes each AC into the source's `// Status:` line as `accepted online (#ID, LANG, DATE UTC, MS ms; the expanded bundle of commit HASH[+dirty], SHA-256 H16)`. A source is `current` while its bundle, Status lines aside, equals the recorded submission, else `stale`. Submissions made by hand are recorded in the same form.

The expander runs from any working directory, discovers unexpanded C++ sources recursively under `src` only, and maps `name.cpp` to `name.expanded.cpp`, preserving relative directories. It bundles through `09-Contest Testing/10-grab.py` by default (user decision 2026-11-05): each library include becomes the template (and debug) header whole, then the declarations the source names from those includes' closures, with `#line` directives naming each origin, since a whole-header bundle can exceed Codeforces' 64 KB source limit; an aggregate include stands for its headers, and any other quoted include outside the library is an error. `--bundler oj-bundle` reproduces the whole-header output; report how to install it when it is missing. Expanded outputs are never inputs, and original sources are never modified. A successful expansion replaces its output atomically; a failed one keeps the prior valid output and exits nonzero.

Cleanup is on by default: `--noclean` keeps obsolete generated outputs, and `--clean` selects the default explicitly. It deletes, through a manifest, only owned generated outputs whose source no longer exists, never other files or sources, and nothing when discovery or bundling fails. Generated files, metadata and build artifacts stay out of version control: judge sources are canonical, generated files reproducible derivatives.

If a Python bundler is introduced, Python programs follow the same separation of sources and outputs, preserving import semantics, with tested expansion; the C++ bundler never claims to handle Python. Standalone Python submissions may stay under `src`.

`99-Workspace` has one permanent file, `template.cpp`: a standalone generated snapshot of `01-Core/01-template.hpp` and `02-debug.hpp`, opening with the motto line, without local includes, `#pragma once` and doc-comment lines, followed by a multi-case driver: `solve(int t)` and `main()` with `ios::sync_with_stdio(false); cin.tie(nullptr); int t = 1; cin >> t;` and `solve(i)` for `i = 1..t`. The generator lives elsewhere, and regeneration is an explicit command, never a side effect of tests, builds or expansion. Temporary contest files are user work, never cleanup targets, and no other permanent helper or config file belongs in the workspace.
