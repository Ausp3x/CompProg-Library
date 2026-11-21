# Speed comparison — evidence (P005)

`06-compare.py` runs the A/B comparison `05-testing.md` (Benchmarks) requires for a speed change to a Core full header: the working header against its previous version, on the deciding compiler in three builds and, as a gate, on the floor compiler and the local one. It replaces the scratch scripts P005's sweep used (user decision 2026-11-17), so the comparison behind a verdict can be rerun by anyone.

## What it does

- **Sides.** `base` is `01-Core` exported from a git ref (`--base`, default `HEAD`), `work` the working tree. `--side NAME=DIR[,MACRO...]` adds a directory of headers that comes first on the include path, with `-DMACRO` options; a prototype with switches is compared this way before the header is written.
- **Builds.** The deciding compiler is the first of `--compilers` (default `g++-15,g++-14,g++`; `03-cpp.md`, Toolchain and judges). Each side is built with it three times: `-O2` (scalar), `-O2 -mavx2 -mbmi2 -mfma` (ISA) and `-O2 -falign-functions=64 -falign-loops=32` (layout). `--gate` adds an `-O2 -march=native` build on the deciding compiler, which the summary lists as a group of its own (`g++-15 native`), and the scalar build on every further compiler. Builds go through the test runner's `Builder` (ccache, at most three at once inside the memory cap).
- **Timing.** Every binary runs once per pass, interleaved, pinned to one CPU with `taskset` (`--core`, default 5). A row's value is its minimum cycle count over the passes (`--passes`, default 3), with that pass's instruction and misprediction counts. Interleaving and the minimum remove the thermal drift this machine shows under sustained load.
- **Checks.** A row's checksum must agree between all sides of a build and between passes, and all binaries must print the same rows; otherwise the run fails and writes no log.
- **Summary.** Per side and build: the geometric mean of the ratios to `--against` (default `base`) and the number of rows more than `--loss` (default 3%) slower or faster. Per compiler: `losing` rows are over the line in every build of that compiler, `layout` rows in only some, which marks them as code placement and not as the change. `show <log.json>` prints the summary of a recorded run again.
- **Record.** The log is `<rig stem>.json` beside the rig (git-ignored). The last summary line starts with `Comparison:` and holds the date, the blobs of the rig and of every `01-Core` header the rig includes, the base commit, the compiler versions, the CPU, the passes and the row and losing-row counts. It goes into the header's evidence document under `## Benchmarks`; `/finish` (`gaps.py`, Comparison record) reports it stale when a compiler version or one of those blobs has changed (`05-testing.md`, Stale evidence).

## Writing a rig

A rig is `96-Local Testing/01-Core/<NN-name>_compare.cpp`. It includes the header under test by its file name (`#include "05-modint.hpp"`, so each side's copy is found first) and [`00-perf.hpp`](../00-perf.hpp), and calls `Rows::run(type, name, per, gen, f, chk)` once per row: `gen` prepares the input from `rows.rng` outside the timed region, `f` is the kernel, `chk` digests its output into the checksum, and `per` is the number of units of work in one `f()`, so the counts print per element or per call. The row's generator is seeded from its name, so every side and build sees the same data. A row runs 3 warm-up and 15 timed repetitions and prints the repetition with the fewest cycles as `type|row|cycles|instructions|misses|checksum`. The first program argument is a comma-separated filter matched against `type row`.

`Rows::net(type, name, per, gen, f, base, chk)` prints the counters of `f` less those of `base`, timed in the same repetition. It serves the evicting row of a lookup table: `base` touches 64 KB (1024 cache lines, more than a 48 KB first-level cache holds) before every call and `f` does the same and then calls the operation, so the row is the operation's cost with its table out of the first-level cache. The difference of two minima is noisier than a plain row.

Rows follow the benchmark matrix (`05-testing.md`, Matrix first): a branchy kernel reads a pool of random operands, a dependent chain and independent calls are separate rows, and an input whose cost depends on a data class (zero, residue or non-residue, sign) has a predictable row and a mixed one.

## Limits

- The counters are user-space hardware counters of this machine (`perf_event_open`; `kernel.perf_event_paranoid` at most 2). They say nothing about a judge's CPU: `05-testing.md` (Other CPUs) states what carries over.
- A row cheaper than about one cycle per unit moves by several percent with code placement alone; the layout build and the `layout` list are the tool's only defence, and such a row is read with its instruction count.
- The tool does not decide: verdicts follow `05-testing.md` (Benchmarks, Mixed rows) and are recorded by hand in the candidates table.

## Tester

[`00-Tools/18-compare_tester.py`](../00-Tools/18-compare_tester.py): its docstring lists every test. The quick tests use fixtures and fake binaries; the full run builds a small rig with the real compiler against the real tree and checks that a side slowed eightfold is listed as losing in every build.

```bash
python3 '96-Local Testing/00-Tools/18-compare_tester.py' --mode full                 # PASS, 10 tests, beside a running build (2026-11-18, g++ 16.2)
```

## History

- 2026-11-17 (P005, `/finish C03`): written from the sweep's scratch scripts.
- 2026-11-18 (P005, `/finish C03`): `--gate` adds the `-march=native` build as its own group (user decision); `Rows::net` for evicting rows; 10 tests passed.
