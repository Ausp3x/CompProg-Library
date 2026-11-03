# Mutation testing of the testers — evidence (P251)

Batch SUP20 ([decisions](../../00-Guidelines/10-decisions.md), 2026-10-31 Model and prompt review). One campaign was run on one Contest header, to show whether a tester's oracles catch injected faults; whether to extend it is the user's decision (Handoff).

## Tool selection

| Candidate | Version and pin | Licence | Fit with `g++` | Verdict |
|---|---|---|---|---|
| [mewt](https://github.com/trailofbits/mewt) (Trail of Bits) | 4.0.0, release commit `53b5d07` (tag `v4.0.0`, 2026-06-18) | AGPL-3.0 | Source-level: tree-sitter C++ grammar, extensions `cpp cc cxx c hpp hxx`; the test command is any shell command, so the compiler is irrelevant | Used |
| [muton](https://github.com/trailofbits/muton) | 3.1.0, `00b4aca` | AGPL-3.0 | Same engine as mewt, targets TON (FunC/Tact/Tolk) only | Not applicable |
| [Mull](https://github.com/mull-project/mull) | 0.36.0 (tag `095f43f`), read at main `62222ff` | Apache-2.0 | Finds mutations through the Clang AST and injects them through an LLVM IR pass plugin (`docs/HowMullWorks.rst`), so the code must be built with clang; clang is not a library target (`03-cpp.md`) and is not installed | Rejected |
| Trail of Bits [`mutation-testing` skill](https://github.com/trailofbits/skills/tree/main/plugins/mutation-testing) | plugin 1.9.2, `trailofbits/skills@d016e08` | CC-BY-SA-4.0 | Drives mewt or muton; the skill content is Markdown (`allowed-tools: Read Write Bash Grep`), and the `tests/` and `evals/` scripts run only by hand | Read, not installed (user decision 2026-11-03); its equivalence procedure (`references/equivalent-mutants.md`) is applied by hand below |

mewt facts read at the pinned commit: `src/` opens no socket and makes no HTTP request (its only URLs are two SARIF metadata strings in `src/core/cmds/results.rs`; `Cargo.toml` has no HTTP crate, although `tokio` is built with `full` features), and the campaign uses only a local `mewt.sqlite` beside `mewt.toml`. mewt writes each mutant **into the target file in place** and restores it after each test or on Ctrl-C (`docs/how-it-works.md`), so it never runs in the checkout. The skill plugin installs no hooks; its `tests/validate_report.py`, `tests/test_validate_report.py` and `tests/smoke-test.sh` (which makes API calls), `evals/` and `agents/openai.yaml` are used only by hand. Installation (user decision 2026-11-03): the release tarball `mewt-x86_64-unknown-linux-gnu.tar.xz`, SHA-256 `0e30678a7d090d112b9cf4d2d620053ca9da7c458ed5c2d94010304edded728b` (checked against the release's `.sha256`, which shows the download is intact, not that the release binary was built from `53b5d07`), unpacked into the session scratch directory; `mewt-installer.sh`, which edits `PATH` and shell rc files, was not run. Nothing was installed system-wide and no file of the tool is in the repository.

## Procedure

1. `git archive HEAD | tar -x -C <scratch>/repo`, so the in-place mutation never touches the checkout.
2. `<scratch>/repo/mewt.toml`:

   ```toml
   [targets]
   include = ["02-Data Structures/01-dsu.hpp"]

   [test]
   cmd = "python3 '96-Local Testing/02-Data Structures/01-dsu_tester.py' --mode full --seed 1 --configuration optimized"
   timeout = 60
   ```

3. Stage 1, every mutant against the fast `-O2 -DNDEBUG` build: `python3 '96-Local Testing/_00_memory_cap.py' -- mewt run --comprehensive`. The cap wraps mewt and every test it starts (`CP_MEMORY_CAPPED` makes the nested tester skip its own scope).
4. Stage 2, survivors against all three builds (the checked build adds the `--invalid` assertion probes): `mewt results --format ids > ids.txt`, then `python3 '96-Local Testing/_00_memory_cap.py' -- mewt test --ids-file ids.txt --test.cmd "python3 '96-Local Testing/02-Data Structures/01-dsu_tester.py' --mode full --seed 1 --configuration all" --test.timeout 240`.
5. Classify every survivor with the five checks of the skill's procedure (type context, reachability, downstream use, semantic comparison, distinguishing input); close each test gap in the tester, copy the tester into the scratch tree and repeat stage 2 on the remaining survivors.
6. `diff` the scratch header against the checkout after each stage (identical every time).

## Target and results

Target (user decision 2026-11-03): [`02-Data Structures/01-dsu.hpp`](../../02-Data%20Structures/01-dsu.hpp) (47 lines, row verified), tester [`01-dsu_tester.cpp`](../02-Data%20Structures/01-dsu_tester.cpp). mewt generated 140 mutants from 11 of its 21 C++ operators (ER 30, CR 30, COS 55, RDV 6, AS 4, AAOS 4, IF 3, IT 3, WF 2, BL 2, LOS 1; 30 high, 42 medium, 68 low severity); `--comprehensive` tested all of them, 135 to an outcome and 5 to a timeout.

| Stage | Tester | Mutants run | Wall time | Peak memory | Caught | Timeout | Survived |
|---|---|---|---|---|---|---|---|
| 1, optimized build | original | 140 | 757 s | 428 MB | 99 | 5 | 36 |
| 2, all builds | original | 36 | 621 s | 545 MB | 20 | 0 | 16 |
| 2, all builds | final (after review) | 16 | 257 s | 611 MB | 10 | 0 | 6 |

Final: 129 caught, 5 timeouts, 6 survivors, all equivalent. The 20 caught in the first stage-2 run are assertion mutants on lines 12, 16 and 28, which the `-DNDEBUG` build cannot see: 4 (`CR 31`, `CR 48`, `LOS 79`, `COS 107`) fail an `--invalid` probe, and 16 tighten the assertion so that it fires during the checked build's ordinary full run. The 5 timeouts (`COS 112, 114` on line 30, `COS 115, 117, 119` on line 31) turn the root walk or the compression loop into an infinite loop at a root (`r <= par[r]` holds at `r == par[r]`; `u == r` holds when `u` is the root); the runner's 180 s subprocess timeout would fail the suite, so they count as caught.

### Test gaps closed

| Mutants | Line | Why the original tester missed them | Closing test |
|---|---|---|---|
| `CR 43`, `IF 62`, `IT 65`, `COS 95`, `COS 96`, `COS 98`, `COS 99` | 23, `if (siz[u] < siz[v]) { swap(u, v); }` | Removing or inverting union by size keeps every connectivity answer right; only the documented `O(log(n))` worst case breaks, and no test looked at tree shape | `checkDepths`: `par` has `n` entries in `[0, n)`, and a vertex at depth `k` in `par` lies in a component of at least `2^k` vertices (the union-by-size invariant, kept by compression). It runs in every `verify`, before that compresses paths, and after two 65,536-vertex stars, `uniteSets(i, 0)` and `uniteSets(0, i)` for every `i`, which alone kill all seven (header depth 1; mutant depth above 65,000). Small distinguishing input: `uniteSets(0,1)`, then `uniteSets(2,0)` or `uniteSets(0,2)` on 3 vertices, in the exhaustive histories. |
| `COS 106` | 28, `u < n` → `u != n` | Probes used only `u = -1` and `u = n`; for `u > n` the mutant passes the assertion and `_GLIBCXX_DEBUG` aborts without an assertion message | Probe `find-past`: `DSU(2).findSet(3)` |
| `CR 35`, `COS 87` | 16, `assert(n < INT_MAX)` removed or `<=` | No probe reached `n == INT_MAX` (`2^31` vertices exceed the memory cap) | Probe `makeset-full`: sets the public `n` to `INT_MAX` on `DSU(2)` and calls `makeSet` (a white-box precondition probe; only the checked build runs it) |

### Equivalent survivors

Four are strictly equivalent; two (`COS 97`, `CR 58`) change state the contract leaves unspecified, so they are equivalent under the contract only, and representative choice and `groups()` capacity stay unspecified rather than pinned by a test (user decision 2026-11-03).

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `COS 86`, `n < INT_MAX` → `n != INT_MAX` | 16 | `n` is an `int`, so `n <= INT_MAX` always and the two conditions agree. |
| `COS 97`, `siz[u] < siz[v]` → `<=` | 23 | Under the contract: on equal sizes the other root becomes the parent (`DSU d(2); d.uniteSets(0,1); d.findSet(1)` gives 1 instead of 0), but representatives are arbitrary by contract and the depth invariant holds both ways. |
| `AS 72`, `swap(u, v)` → `swap(v, u)` | 23 | `std::swap` is symmetric. |
| `CR 58`, `res.reserve(ncon)` removed | 40 | Under the contract: only `groups().capacity()` changes, which the contract leaves unspecified; contents and order are identical. |
| `COS 126`, `u < n` → `u != n` | 41 | `u` starts at 0 and steps by 1, and `n >= 0`. |
| `COS 132`, `id[r] == -1` → `id[r] <= -1` | 43 | `id` holds only `-1` or indices in `[0, n)`. |

## Commands and results (2026-11-03)

Machine: AMD Ryzen AI 9 HX 370, Linux 6.18 (WSL2), g++ 16.0.1 20260322 and g++-14 14.3.0, Python 3.14.4; mewt 4.0.0. Strengthened tester (final, after review) on the unchanged header:

```bash
python3 '96-Local Testing/02-Data Structures/01-dsu_tester.py' --mode quick
python3 '96-Local Testing/02-Data Structures/01-dsu_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/01-dsu_tester.py' --mode full --seed 20261103
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 2 --filter '02-Data Structures/01-dsu' --no-integration
python3 '96-Local Testing/03-consistency.py'
```

All passed: quick (2 configurations, 9 assertion probes, peak 389 MB); full on g++ (3 configurations, peak 539 MB) and on g++-14 (peak 521 MB); stress, 2 rounds (peaks 667 and 666 MB). `03-consistency.py` reports no errors. The star cases were confirmed by a scratch driver running only `balancedTree` against each line-23 mutant (all seven fail in `star unions`; `COS 97` passes). The campaign commands and their results are in Procedure and Target and results.

## Limits

- One header, one seed, one tester. A survivor-free result does not show that the oracles are complete for inputs mewt's operators cannot express (wrong algorithms, missing operations).
- Stage 1 runs only the optimized build, so assertion and sanitizer-only faults are judged in stage 2 alone; a mutant caught only by ASan/UBSan would be missed if it also passed stage 1 and stage 2 was skipped.
- The `find-past` kill of `COS 106` depends on the runner (`_00_runner.py`) building this tester's checked build with `_GLIBCXX_DEBUG`, whose out-of-bounds message lacks the word `assert`. Under `-D_GLIBCXX_ASSERTIONS` libstdc++ prints `Assertion '__n < this->size()' failed`, which passes the runner's death check (any SIGABRT whose stderr contains `assert`), and the mutant survives. Tightening that check is a follow-up in P006's plan note (user decision 2026-11-03), with the depth-first reproducer of `unionHistories`.
- Timeouts are counted as caught because each was traced to an infinite loop by hand, not because mewt proves it.
- mewt is AGPL-3.0: running it on the library imposes nothing, but it must never be vendored into the repository.

## Handoff

Cost: about 5.4 s per mutant for stage 1 on this header (140 mutants, 757 s) and about 17 s per survivor for stage 2; of the 36 first-stage survivors, 20 were assertion mutants that only the checked build sees, 10 exposed three real test gaps (union by size never checked; `findSet` never probed with `u > n`; `makeSet` never probed at `n == INT_MAX`) and 6 are equivalent. Not extended for now (user decision 2026-11-03): no further campaign or package; this procedure stays available for a future tester overhaul. For such a run, budget per header roughly 3 mutants per source line and the tester's optimized full-mode runtime per mutant, and run it only after a tester overhaul (mewt's own guidance), never on every change.

## History

- 2026-11-03: P251/SUP20, tool survey, mewt 4.0.0 campaign on `01-dsu.hpp`, three test gaps (10 mutants) closed in `01-dsu_tester.cpp`, 6 equivalent survivors left; independent review, 6 findings, 7 defects fixed (`finish-log.tsv`); quick, full on g++ and g++-14, 2 stress rounds and consistency passed.
