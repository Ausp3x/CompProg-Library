# 050-interactive.hpp — Evidence

`050-interactive.hpp` (batch MI32, package P229) wraps the contestant side of an interactive protocol: formatted query lines, a flush before every read, a query budget and the judge-error exit. Prerequisite: the C01 template (P002); independent contest-profile code.

## Contracts

### Interactive

`Interactive(budget = LLONG_MAX, in = cin, out = cout)` borrows the two streams, which must outlive the object. The fields `budget` and `used` are public; `remaining()` is `budget - used`; `reset(b)` sets a new budget and clears `used` (per test case). `ask` asserts `used < budget` before writing and then counts the query; `answer` never counts, and raw `writeLine` calls are not budgeted.

Line format: `writeLine(args...)` writes every argument as a token, separated by single spaces, and ends the line with `\n`. A range argument (anything satisfying `std::ranges::range` that is not convertible to `string_view`) expands recursively into its elements, so `vector<vector<int>>` flattens; an empty range contributes nothing, so no double or trailing spaces appear. Strings, `string_view`, string literals and `char` are tokens printed with `operator<<`, unquoted. The query or answer prefix (`?`, `!`, `F`, …) is simply the first argument: `io.ask('?', l, r)`, `io.answer('!', v)`. Writes go to the stream buffer; `answer` flushes, `writeLine` does not.

Reads: `readReply<T = lng>()` flushes `out` and extracts one `T` with `in >> x`; `readLine()` flushes, skips whitespace including the rest of the current line and blank lines, reads up to `\n` and strips one trailing `\r`. Both call `judgeErrorExit()` on any extraction failure (EOF, malformed token, overflow): a closed or garbage stream means the judge has already decided, and continuing would only turn the verdict into a crash. `ask<R>(args...)` is `writeLine` plus `readReply<R>()`; multi-token replies and the verdict after `answer` use `readReply` or `readLine` directly.

Error sentinel: `error` is a `std::optional<lng>`, default `-1`. When it is set, a signed integer reply wider than one byte (`int`, `lng`, `short`, …) equal to it triggers `judgeErrorExit()`; token, line, floating-point and one-byte replies are never checked (`char`, `int8_t` and `uint8_t` extract one character, not a number). Unsigned reply types cannot represent the judge's `-1` (libstdc++ reads the text `-1` as all-ones), so `readReply<ulng>()` and `ask<uint>()` **assert that the sentinel is disabled**: read verdicts through a signed type, or call `error.reset()` first. `error.reset()` also serves protocols where `-1` is a valid reply, and `error = 0` or any other value adapts the sentinel. `std::optional` holds the one setting that a flag plus a value would split into two fields.

`judgeErrorExit()` flushes `out` and calls `std::exit(0)`; it never returns. The wrapper never touches `std::ios::sync_with_stdio` or `cin.tie`, and it is correct with the template's untied, unsynchronized streams because it flushes before every read. It is not thread-safe; use one object per protocol.

### Legacy Mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `flushNow()` | `io.out.flush()` on the public `out` stream |

## Feature-to-Test Map

| Public feature | Coverage |
|---|---|
| `writeLine`, `ask`, `answer` formatting | Fixed cases against hand-written expected bytes: `char`, string literal, `string`, `string_view`, `int`, `lng`, `double`, `vector<int>`, empty `vector`, nested `vector<vector<int>>` with an empty inner vector, `array`, `set`, `vector<string>`, empty line, prefix only; seeded random transcripts (200, 5,000 or 50,000 rounds) rebuilt independently. |
| `ask`, `ask<T>`, `used`, `remaining`, `budget`, `reset` | Return types `lng`, `string`, `int`, `double`; counts after each ask; exact exhaustion at the budget; `answer` not counted; `reset` clears `used` and sets the budget; `budget-zero` and `budget-exceeded` assertion probes. |
| Flush before every read | Instrumented streams: an output buffer that publishes only on `sync` and an input buffer that serves one byte at a time and records any byte read while output was pending. Every fixed and random case, raw `io.out << ...` writes included, must show zero violations, and an explicit `io.out.flush()` publishes a pending line before any read. Removing the flush from `readReply` fails this check and stalls the pipe game; removing the explicit `io.out.flush()` fails `out.flush publishes`. |
| `readReply` (int, token), `readLine` | `LLONG_MIN`, `LLONG_MAX`, token `"-1"`, a disabled sentinel returning `-1`, unsigned replies with the sentinel disabled (`ULLONG_MAX`; text `-1` read into `uint` is all-ones), `char`; an unsigned reply with an active default or custom sentinel is the `unsigned-sentinel` or `unsigned-sentinel-custom` assertion probe; `readLine` skips blank lines and leading spaces, keeps inner and trailing spaces, strips `\r`, and works after a token read. |
| `judgeErrorExit`, error sentinel, EOF and malformed input | Subprocess scenarios on real stdio, checking exit status 0 and the exact flushed output: integer `-1`, custom sentinel `0`, verdict `-1` after `answer`, EOF on an int, token or line read, malformed token, `int` overflow. Disabling the sentinel test fails two scenarios and the pipe game. |
| Live protocol | Python interactor over pipes with `cin` untied and stdio sync off: seeded multi-test binary search (n up to 1e18) with `reset` per case and the `ceil(log2 n)` query limit enforced by the interactor, echo of a `readLine` line with spaces and `\r`, verdict reads, and a `-1` mid-case expecting a silent exit 0; 3, 20 or 60 games, 30 s watchdog. |

## Commands and Results

Conditions: Intel Core i9-11900H, Linux x86-64, GNU++20, CPython 3.14.7, GCC 16.2.1 20260810 (`g++`) and GCC 14.4.1 20260915 (`CXX=g++-14`); optimized, checked and ASan/UBSan builds made by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>). The 2026-11-10 library-wide runs used seed 42, ran beside the other suites and emptied the build cache before each cold run; their times are this tester's.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 737 MB; optimized 0.1 s compile + 0.3 s run, checked 13.3 s + 0.5 s, ASan-UBSan 14.6 s + 3.0 s; peak 866 MB |
| 2026-11-10 (P274) | `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 764 MB; optimized 0.3 s + 0.4 s, checked 17.1 s + 0.6 s, ASan-UBSan 19.4 s + 2.2 s; peak 907 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode quick` | PASS; optimized 17.8 s + 0.1 s, checked 12.5 s + 0.2 s; peak 714 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | PASS; quick preflight peak 738 MB; optimized 0.1 s + 0.5 s, checked 4.9 s + 0.8 s, ASan-UBSan 5.3 s + 5.5 s; peak 461 MB |
| 2026-10-13 (P229 prune) | `python3 '96-Local Testing/06-Miscellaneous/050-interactive_tester.py' --mode full --seed 1` | PASS, 3 configurations, 69,489 checks, 8 scenarios, 20 games, 4 assertion probes, peak 415 MB |
| 2026-10-13 (P229 prune) | `CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/050-interactive_tester.py' --mode full --seed 2` | PASS, 3 configurations, 68,710 checks, 8 scenarios, 20 games, 4 assertion probes, peak 428 MB |
| 2026-10-13 (P229 prune) | `python3 '96-Local Testing/03-consistency.py' --braces 06-Miscellaneous/050-interactive.hpp '96-Local Testing/06-Miscellaneous/050-interactive_tester.cpp'` | no violations |
| 2026-10-13 (P229 prune) | `python3 '96-Local Testing/02-integration.py'` | PASS, 135 standalone and aggregate headers, multi-TU, workspace; the header is warning-free with `-Wall -Wextra -Wconversion` on GCC 16.2 and 14.4 in a two-TU build through `99-all.hpp` |
| 2026-10-13 (P229 prune) | `python3 '96-Local Testing/03-consistency.py'` | no errors |

## Benchmarks

None: every operation is one formatted line or one extraction, and interactive problems are bound by the judge round trip.

## Sources

Catalog sweep by `@researcher` on 2026-10-08 (ledger in [00-sources.md](00-sources.md)); the code was written independently:

- CSES [3139](https://cses.fi/problemset/task/3139) and [3305](https://cses.fi/problemset/task/3305), AtCoder [practice_2](https://atcoder.jp/contests/language-test-202001/tasks/practice_2): the `? …`/`! …`, `F i`/`S i` and `<`/`>` protocols motivate the free prefix argument and range expansion.
- The Codeforces interactive-problem guide (blog 45307) and the 1999G1 statement, read 2026-11-22 ([ledger](00-sources.md#fetched-2026-11-22-source-check)). The guide: the judge's reply can be read only after the program flushes, so flush after every output. 1999G1: the query limit holds per test case, and an extra or invalid query or a wrong answer is answered `-1`, after which the program must terminate at once, since reading on would hit a closed stream. These back the flush before every read, `reset` per test and `judgeErrorExit`. Neither page sends a verdict after a correct answer: in 1999G1 the next test simply begins, and a wrong answer surfaces as `-1` at the next read; the verdict reads in the contract serve protocols that do send one. The guide also advises `scanf`/`printf` over `cin`/`cout` for speed; the wrapper keeps `iostream`, because the round trip bounds these problems (Benchmarks).

## Limits and Handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): `readVerdict`, `readBool`/`readCmp`, comparator adapters, `readVec(n)`, a local interactor (that is `09-Contest Testing/05-interactor.py`).
- Buffered `FastInput<N>` readers (`01-Core/19-fastio.hpp`) with `N > 1` are not used, since bulk `fread` can block on a live pipe. `iostream` with the flush discipline above is the portable choice; `FastInput<1>` is that header's interactive form.
- `08-Python` names its counterpart `ask`/`tell`/`finish`; the C++ names follow the inventory row (`ask`/`answer`).
- Open for the package's next audit: `std::istream`/`std::ostream` qualification, and the C-style cast `(unsigned char)ch` in the tester.
- `\r` stripping covers Windows line endings that reach the stream untranslated.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): 交互题.

## History

- 2026-10-08: created and verified (P229); `@reviewer` found 1 defect, fixed; quick, full on g++ and g++-14, stress with seed 3 and integration with sanitizers passed.
- 2026-10-13: prune removed `flushNow` (its case moved to `io.out.flush()`); full passed on g++ and g++-14, integration and consistency clean; `@reviewer` found no correctness defect, 4 findings fixed.
- 2026-11-07: P263 added Chinese search aliases (OI Wiki page titles and common names of the row's operations, Simplified plus Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 (SUP25) moved the builds to the shared runner `_00_contest_runner.py` and its ccache `Builder`, with warnings `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; the library-wide runs above passed. For `-Werror` the header gained `[[maybe_unused]]` on names only an `assert` uses, with no logic change; no mutation campaign in P274 (user decision 2026-11-10), the first campaign stays with the package.
