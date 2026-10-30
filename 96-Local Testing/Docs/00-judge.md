# Library Checker judge tool — evidence

Scope: [`07-judge.py`](../00-Tools/07-judge.py) fetches the fastest submissions, regenerates the judge's tests and times solutions for `/speed-sweep` (`00-Guidelines/09-sources.md`, Fastest submissions; user decision 2026-10-25). Its tester is [`07-judge_tester.py`](../00-Tools/07-judge_tester.py).

## Design

- **API.** GET requests to `https://v3.api.judge.yosupo.jp`. Every response is stored under `.judge-cache/api/`. A submission's details are reused once its status is final, while lists are refetched unless `--offline` is given. Sources are written to `.judge-cache/sources/<id>.<ext>`.
- **Distinct code.** `fastest` merges a submission into an earlier kept one when their line multisets have a Jaccard index of at least 0.9. For C++, comments and whitespace are dropped first, while string literals are kept. On 2026-10-25 this merged Rohan_Kapri's #406184 with AZZRCN's anonymous #391745 on hex addition.
- **Tests.** `reproduce` takes the problem from `--repo`, or else from a sparse clone in `.judge-cache/library-checker-problems`, adding the problem's `source_url` path from the API. It mirrors upstream `generate.py`:
  - `params.h` is written from `[params]`;
  - generators, `sol/correct.cpp` and the checker are built with `g++ -O2 -std=c++17 -I common`;
  - `.in` tests are copied and `.cpp` generators run with the case index, giving `<stem>_NN` cases in `info.toml` order;
  - reference answers come from `correct`.

  The tests are rebuilt when `info.toml` changes (SHA-1 stamp), in a temporary directory that is then renamed into place. Function-style (grader) problems are refused.
- **Timing.** The solution is built with the judge's flags (`CXX`, default `g++`; `-march=native` only for an x86-64 target, so a cross compiler builds it too). A static launcher, compiled per run, forks and execs it and reports:
  - the monotonic wall time;
  - `ru_maxrss`, which no longer includes the Python parent's image (the first version reported 18.1 MiB for every hex-addition case, the launcher reports 4.1–4.3);
  - the exit status.

  Each test takes the minimum over `--runs` (default 3). The CPU limit is 2 × the time limit + 1 s. The status is AC through the problem's checker, otherwise WA, RE, or TLE (wall time above the limit, or SIGXCPU/SIGKILL). The whole reproduction runs under `_00_memory_cap.ensure(force=True)`, 4096 MB by default (`--mb`).

  Results go to `.judge-cache/results/<problem>/<label>.json` with the CPU, compiler, flags, runs, tests commit and the `untrusted` flag.
- **Untrusted sources.** `reproduce` refuses a path under `.judge-cache/sources` unless `--untrusted` is given (user decision 2026-10-25: opt-in, with the user's yes per sweep). Both the build and the timing loop (`07-judge.py _time`) then run under `bwrap`:
  - `/` is read-only, with a private `/tmp` and every namespace unshared, so there is no network;
  - `96-Local Testing` and the tests are bound read-only;
  - only the work directory is writable.

  Without `bwrap` the flag is refused.
- **Compare.** A Markdown table per case:
  - our local and judge times, then each leader's judge time and, when recorded, its local time;
  - `judge gap` (ours ÷ best leader on the judge) and `local gap` (ours ÷ best leader locally);
  - `max` (the score) and `sum` rows;
  - our local max × 1.3 as a judge estimate.
- **Cache.** `96-Local Testing/.judge-cache` is git-ignored (`CP_JUDGE_CACHE` overrides it). It is per worktree, and `03-consistency.py` skips it.

## Feature-to-test map

| Feature | Tests |
|---|---|
| Comment and whitespace normalisation, string literals kept, similarity values | `test_similarity`, `test_similarity_random` (50 seeded cases; `--rounds` × 50 in stress) |
| `fastest`: distinct-code merge, table, details fetched once, lists refetched, entries without `submission_time` | `test_fastest_dedupe` |
| `submission`: cached source outside the tracked tree, per-case table, pending status refetched | `test_submission_cache` |
| `--offline`, `problems` | `test_offline`, `test_problems` |
| `mine`, newest AC past a newer WA | `test_mine` |
| `compare`: columns, cells, non-AC marks, judge-only cases, gaps, max and sum, estimate, `--user` | `test_compare` |
| `reproduce`: params.h, generator inputs against a Python oracle, answers, case order, AC, min of three runs, small RSS (8 MB, or 64 MB when `native()` is false and qemu-user's own RSS counts), work directory removed, CLI | `test_reproduce_ok` |
| WA, RE, TLE by wall time, TLE by CPU limit, build failure, grader refusal | `test_reproduce_failures` |
| Regeneration only when `info.toml` changes | `test_regeneration_stamp` |
| Fetched-source refusal; `--untrusted` under bwrap: read-only tests and no network, proven from inside the solution; result flag | `test_untrusted` (SKIP without bwrap) |

Mutation checks on 2026-10-25 (a copy outside `/tmp`), each caught by the named test:

| Mutation | Caught by |
|---|---|
| network kept | `test_untrusted` |
| tests writable | `test_untrusted` |
| max instead of min | `test_reproduce_ok` |
| merge threshold above 1 | `test_fastest_dedupe` |
| pending details cached | `test_submission_cache` |
| CPU-limit kill read as RE | `test_reproduce_failures` |
| string literals stripped | `test_similarity` |

## Commands and results

2026-10-25 (GCC 16.2, GCC 14.4.1, CPython 3.14, i9-11900H, bwrap installed; the host had a load average of about 4.5 from other sessions); the `07-judge_tester.py` rows are from 2026-10-30:

| Command | Result |
|---|---|
| `python3 '96-Local Testing/00-Tools/07-judge_tester.py' --mode quick` | 8 tests, 2.2 s, PASS, peak 70 MB |
| `python3 '96-Local Testing/00-Tools/07-judge_tester.py' --mode full` | 12 tests, 9.8 s, PASS, peak 81 MB |
| `python3 '96-Local Testing/00-Tools/07-judge_tester.py' --mode stress --seed 7 --rounds 3` | PASS, peak 83 MB |
| `CXX=g++-14 python3 '96-Local Testing/00-Tools/07-judge_tester.py' --mode full` | PASS, peak 72 MB |
| `CXX=aarch64-linux-gnu-g++ QEMU_LD_PREFIX=/usr/aarch64-linux-gnu python3 '96-Local Testing/00-Tools/07-judge_tester.py' --mode quick` | PASS, peak 70 MB (qemu-user) |
| live: `fastest addition_of_hex_big_integers`, `mine Ausp3x …`, `reproduce … '97-Online Testing/src/01-Yosupo/02-Big Integers/07-additionhex.cpp' --label ours`, `compare … --user Ausp3x --top 3` | 29 distinct entries with the copy merged; 24 tests AC, local max 50 ms (`small_00`), reproduction peak 548 MB; judge gap 4.89 on `small_00` |

The launcher and a direct Python spawn timed the same binary within noise: `small_00` 39.3 against 39.4 ms, `medium_00` 5.56 against 5.83 ms (minimum of 15).

## Limits

- Library Checker only. Function-style problems and `.py` generators with arguments are not covered: upstream passes `.py` generators no index, and the tool does the same.
- The tests are those of the cached checkout's commit, which is recorded but not compared with the API's `testcases_version`.
- bwrap does not limit CPU time beyond the RLIMIT, or the process count.

## History

- 2026-10-30: SUP18 portability: `-march=native` only for an x86-64 target, the RSS bound widened under qemu-user; quick, full, stress, g++-14 and arm64 quick passed.
- 2026-10-25: tool, tester and sandbox built from the P018 re-check prototypes; the launcher replaced Python-side `wait4` after the live run showed the inherited RSS; tester passed in quick, full, stress and on g++-14.
- 2026-10-26: `fastest division_of_big_integers` raised `KeyError: 'submission_time'` during the P018 speed sweep (three 2023 entries carry no time stamp); `date()` now prints an empty day for them, the tester covers it, quick and full reran (user's yes, recorded here per scope discipline).
