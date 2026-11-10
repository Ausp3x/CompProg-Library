# 50-interactive.hpp — evidence

`50-interactive.hpp` (batch MI32, package P229) wraps the contestant side of an interactive protocol: formatted query lines, a flush before every read, a query budget and the judge-error exit. Prerequisite: the C01 template (P002). Independent contest-profile code.

## Contracts

### Interactive

`Interactive(budget = LLONG_MAX, in = cin, out = cout)` borrows the two streams; they must outlive the object. Fields `budget` and `used` are public; `remaining()` is `budget - used`; `reset(b)` sets a new budget and clears `used` (per test case). `ask` asserts `used < budget` before writing, then counts the query; `answer` never counts. Only `ask` is budgeted; raw `writeLine` calls are not.

Line format: `writeLine(args...)` writes every argument as a token separated by single spaces and ends the line with `\n`. A range argument (anything satisfying `std::ranges::range` that is not convertible to `string_view`) is expanded into its elements, recursively, so `vector<vector<int>>` flattens; an empty range contributes nothing, so no double spaces or trailing spaces appear. Strings, `string_view`, string literals and `char` are tokens, printed with `operator<<` (no quoting). The query or answer prefix (`?`, `!`, `F`, …) is just the first argument: `io.ask('?', l, r)`, `io.answer('!', v)`. Writes go to the stream buffer; `answer` flushes, `writeLine` does not.

Reads: `readReply<T = lng>()` flushes `out`, then extracts one `T` with `in >> x`; `readLine()` flushes, skips whitespace including the rest of the current line and blank lines, reads up to `\n` and strips one trailing `\r`. Both call `judgeErrorExit()` on any extraction failure (EOF, malformed token, overflow): a closed or garbage stream means the judge has already decided, and continuing would only replace the verdict with a crash. `ask<R>(args...)` is `writeLine` plus `readReply<R>()`; use `readReply`/`readLine` directly for multi-token replies and for the verdict after `answer`.

Error sentinel: `error` is `std::optional<lng>`, default `-1`. When set, a signed integer reply wider than one byte (`int`, `lng`, `short`, …) equal to it triggers `judgeErrorExit()`; token, line, floating-point and one-byte replies are never checked (`char`, `int8_t` and `uint8_t` extract a single character, not a number). Unsigned reply types cannot represent the judge's `-1` (libstdc++ turns the text `-1` into all-ones), so `readReply<ulng>()`/`ask<uint>()` **assert that the sentinel is disabled**: read verdicts through a signed type, or `error.reset()` first. `error.reset()` also serves protocols where `-1` is a valid reply; `error = 0` or any other value adapts it. `std::optional` is used because a flag plus value would be two fields for one setting.

`judgeErrorExit()` flushes `out` and calls `std::exit(0)`; it never returns. The wrapper never touches `std::ios::sync_with_stdio` or `cin.tie`; it is correct with the template's untied, unsynchronized streams because it flushes before every read. Not thread-safe; one object per protocol.

### Legacy mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `flushNow()` | `io.out.flush()` on the public `out` stream |

## Feature-to-test map

| Public feature | Coverage |
|---|---|
| `writeLine`, `ask`, `answer` formatting | Fixed cases against hand-written expected bytes: `char`, string literal, `string`, `string_view`, `int`, `lng`, `double`, `vector<int>`, empty `vector`, nested `vector<vector<int>>` with an empty inner, `array`, `set`, `vector<string>`, empty line, prefix only; seeded random transcripts (200/5,000/50,000 rounds) rebuilt independently. |
| `ask`, `ask<T>`, `used`, `remaining`, `budget`, `reset` | Return types `lng`, `string`, `int`, `double`; counts after each ask; exact exhaustion at the budget; `answer` not counted; `reset` clears `used` and sets the budget; `budget-zero` and `budget-exceeded` assertion probes. |
| Flush before every read | Instrumented streams: an output buffer that publishes only on `sync` and an input buffer that serves one byte at a time and records any byte read while output was pending; zero violations required in every fixed and random case, including raw `io.out << ...` writes; an explicit `io.out.flush()` publishes a pending line before any read. Mutation checks: removing the flush from `readReply` fails this check and stalls the pipe game; removing the explicit `io.out.flush()` fails `out.flush publishes`. |
| `readReply` (int, token), `readLine` | `LLONG_MIN`, `LLONG_MAX`, token `"-1"`, disabled sentinel returning `-1`, unsigned replies with the sentinel disabled (`ULLONG_MAX`; text `-1` into `uint` is all-ones), `char`; unsigned reply with an active default or custom sentinel are the `unsigned-sentinel` and `unsigned-sentinel-custom` assertion probes; `readLine` skipping blank lines and leading spaces, keeping inner/trailing spaces, stripping `\r`, after a token read. |
| `judgeErrorExit`, error sentinel, EOF and malformed input | Subprocess scenarios on real stdio, exit status 0 and exact flushed output: integer `-1`, custom sentinel `0`, verdict `-1` after `answer`, EOF on int/token/line, malformed token, `int` overflow. Mutation check: disabling the sentinel test fails two scenarios and the pipe game. |
| Live protocol | Python interactor over pipes with `cin` untied and stdio sync off: seeded multi-test binary search (n up to 1e18) with `reset` per case, `ceil(log2 n)` query limit enforced by the interactor, echo of a `readLine` line with spaces and `\r`, verdict reads, and a `-1` mid-case expecting a silent exit 0; 3/20/60 games, 30 s watchdog. |

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 737 MB; optimized 0.1 s compile + 0.3 s run, checked 13.3 s compile + 0.5 s run, ASan-UBSan 14.6 s compile + 3.0 s run; MEMORY peak 866 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 764 MB; optimized 0.3 s compile + 0.4 s run, checked 17.1 s compile + 0.6 s run, ASan-UBSan 19.4 s compile + 2.2 s run; MEMORY peak 907 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 17.8 s compile + 0.1 s run, checked 12.5 s compile + 0.2 s run; MEMORY peak 714 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 738 MB; optimized 0.1 s compile + 0.5 s run, checked 4.9 s compile + 0.8 s run, ASan-UBSan 5.3 s compile + 5.5 s run; MEMORY peak 461 MB. Dated rows below stay those commands' latest results.

P229 prune run, 2026-10-13: Linux x86-64, i9-11900H, GCC 16.2.1 20260810 and GCC 14.4.1 20260915, CPython 3.14.7. Configurations as in [49-timer.md](49-timer.md).

```bash
python3 '96-Local Testing/06-Miscellaneous/50-interactive_tester.py' --mode full --seed 1              # PASS, 3 configurations, 69,489 checks, 8 scenarios, 20 games, 4 assertion probes, peak 415 MB
CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/50-interactive_tester.py' --mode full --seed 2   # PASS, 3 configurations, 68,710 checks, 8 scenarios, 20 games, 4 assertion probes, peak 428 MB
python3 '96-Local Testing/03-consistency.py' --braces 06-Miscellaneous/50-interactive.hpp '96-Local Testing/06-Miscellaneous/50-interactive_tester.cpp'   # no violations
python3 '96-Local Testing/02-integration.py'                                                           # PASS, 135 standalone and aggregate headers, multi-TU, workspace
python3 '96-Local Testing/03-consistency.py'                                                           # no errors
```

Warning-free with `-Wall -Wextra -Wconversion` under GCC 16.2 and 14.4 in a two-translation-unit build through `99-all.hpp`. The tester build under the same flags gets one `-Wunused-but-set-variable` on `first` in `writeLine` from the argument-free `answer()` case; it predates the prune (Limits and handoffs).

## Benchmarks

None: every operation is one formatted line or one extraction, and interactive problems are bound by the judge round trip.

## Sources

Catalog sweep by `@researcher` on 2026-10-08 (ledger in [00-sources.md](00-sources.md)); code written independently:

- CSES [3139](https://cses.fi/problemset/task/3139) and [3305](https://cses.fi/problemset/task/3305), AtCoder [practice_2](https://atcoder.jp/contests/language-test-202001/tasks/practice_2): `? …`/`! …`, `F i`/`S i` and `<`/`>` protocols motivate the free prefix argument and range expansion.
- Codeforces interactive-problem guide (blog 45307) and 1999G1 statement, seen only in search snippets (direct fetches returned 403): flush before reading, exit after `-1`, per-test budgets, verdict after the answer.

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): `readVerdict`, `readBool`/`readCmp`, comparator adapters, `readVec(n)`, a local interactor (that is `09-Contest Testing/05-interactor.py`).
- Buffered `FastInput<N>` readers (`01-Core/19-fastio.hpp`) with `N > 1` are not used: bulk `fread` can block on a live pipe. `iostream` with the flush discipline above is the portable choice; `FastInput<1>` is that header's interactive form.
- `08-Python` names its counterpart `ask`/`tell`/`finish`; the C++ names follow the inventory row (`ask`/`answer`).
- Open: `writeLine()` with no arguments sets `first` without reading it, so `-Wall` warns at that instantiation; left for the package's next audit (a prune changes only its items).
- Open, also for the next audit: `std::istream`/`std::ostream` qualification, and a C-style cast in the tester.
- `\r` stripping covers Windows line endings that reach the stream untranslated.

## Aliases

Search aliases (P263, grab --find): 交互题.

## History

- 2026-10-08: created and verified (P229), `@reviewer` found 1 defect, fixed; quick, full on g++ and g++-14, stress seed 3 and integration with sanitizers passed.
- 2026-10-13: prune, removed `flushNow` (case moved to `io.out.flush()`); full suite passed on g++ and g++-14, integration and consistency clean; `@reviewer` found no correctness defect, 4 findings fixed.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed. Header edit for `-Werror` without demotions: `[[maybe_unused]]` on names only an `assert` uses; no logic changed; no mutation campaign in P274 (user decision 2026-11-10), its first campaign stays with its own package.
