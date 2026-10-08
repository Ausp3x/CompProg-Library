# 50-interactive.hpp — evidence

`50-interactive.hpp` (batch MI32, package P229) wraps the contestant side of an interactive protocol: formatted query lines, a flush before every read, a query budget and the judge-error exit. Prerequisite: the C01 template (P002). Independent contest-profile code; no online submission was made.

## Contracts

### Interactive

`Interactive(budget = LLONG_MAX, in = cin, out = cout)` borrows the two streams; they must outlive the object. Fields `budget` and `used` are public; `remaining()` is `budget - used`; `reset(b)` sets a new budget and clears `used` (per test case). `ask` asserts `used < budget` before writing, then counts the query; `answer` never counts. Only `ask` is budgeted; raw `writeLine` calls are not.

Line format: `writeLine(args...)` writes every argument as a token separated by single spaces and ends the line with `\n`. A range argument (anything satisfying `std::ranges::range` that is not convertible to `string_view`) is expanded into its elements, recursively, so `vector<vector<int>>` flattens; an empty range contributes nothing, so no double spaces or trailing spaces appear. Strings, `string_view`, string literals and `char` are tokens, printed with `operator<<` (no quoting). The query or answer prefix (`?`, `!`, `F`, …) is just the first argument: `io.ask('?', l, r)`, `io.answer('!', v)`. Writes go to the stream buffer; `answer` flushes, `writeLine` does not.

Reads: `readReply<T = lng>()` flushes `out`, then extracts one `T` with `in >> x`; `readLine()` flushes, skips whitespace including the rest of the current line and blank lines, reads up to `\n` and strips one trailing `\r`. Both call `judgeErrorExit()` on any extraction failure (EOF, malformed token, overflow): a closed or garbage stream means the judge has already decided, and continuing would only replace the verdict with a crash. `ask<R>(args...)` is `writeLine` plus `readReply<R>()`; use `readReply`/`readLine` directly for multi-token replies and for the verdict after `answer`.

Error sentinel: `error` is `std::optional<lng>`, default `-1`. When set, a signed integer reply wider than one byte (`int`, `lng`, `short`, …) equal to it triggers `judgeErrorExit()`; token, line, floating-point and one-byte replies are never checked (`char`, `int8_t` and `uint8_t` extract a single character, not a number). Unsigned reply types cannot represent the judge's `-1` (libstdc++ turns the text `-1` into all-ones), so `readReply<ulng>()`/`ask<uint>()` **assert that the sentinel is disabled**: read verdicts through a signed type, or `error.reset()` first. `error.reset()` also serves protocols where `-1` is a valid reply; `error = 0` or any other value adapts it. `std::optional` is used because a flag plus value would be two fields for one setting.

`judgeErrorExit()` flushes `out` and calls `std::exit(0)`; it never returns. `flushNow()` flushes `out`. The wrapper never touches `std::ios::sync_with_stdio` or `cin.tie`; it is correct with the template's untied, unsynchronized streams because it flushes before every read. Not thread-safe; one object per protocol.

## Feature-to-test map

| Public feature | Coverage |
|---|---|
| `writeLine`, `ask`, `answer` formatting | Fixed cases against hand-written expected bytes: `char`, string literal, `string`, `string_view`, `int`, `lng`, `double`, `vector<int>`, empty `vector`, nested `vector<vector<int>>` with an empty inner, `array`, `set`, `vector<string>`, empty line, prefix only; seeded random transcripts (200/5,000/50,000 rounds) rebuilt independently. |
| `ask`, `ask<T>`, `used`, `remaining`, `budget`, `reset` | Return types `lng`, `string`, `int`, `double`; counts after each ask; exact exhaustion at the budget; `answer` not counted; `reset` clears `used` and sets the budget; `budget-zero` and `budget-exceeded` assertion probes. |
| Flush before every read | Instrumented streams: an output buffer that publishes only on `sync` and an input buffer that serves one byte at a time and records any byte read while output was pending; zero violations required in every fixed and random case, including raw `io.out << ...` writes. Mutation check: removing the flush from `readReply` fails this check and stalls the pipe game. |
| `readReply` (int, token), `readLine` | `LLONG_MIN`, `LLONG_MAX`, token `"-1"`, disabled sentinel returning `-1`, unsigned replies with the sentinel disabled (`ULLONG_MAX`; text `-1` into `uint` is all-ones), `char`; unsigned reply with an active default or custom sentinel are the `unsigned-sentinel` and `unsigned-sentinel-custom` assertion probes; `readLine` skipping blank lines and leading spaces, keeping inner/trailing spaces, stripping `\r`, after a token read. |
| `judgeErrorExit`, error sentinel, EOF and malformed input | Subprocess scenarios on real stdio, exit status 0 and exact flushed output: integer `-1`, custom sentinel `0`, verdict `-1` after `answer`, EOF on int/token/line, malformed token, `int` overflow. Mutation check: disabling the sentinel test fails two scenarios and the pipe game. |
| Live protocol, `flushNow` | Python interactor over pipes with `cin` untied and stdio sync off: seeded multi-test binary search (n up to 1e18) with `reset` per case, `ceil(log2 n)` query limit enforced by the interactor, echo of a `readLine` line with spaces and `\r`, verdict reads, and a `-1` mid-case expecting a silent exit 0; 3/20/60 games, 30 s watchdog. |

## Commands and results

Run 2026-10-08: Linux x86-64, i9-11900H, GCC 16.2.1 and GCC 14.4.1, CPython 3.14. Configurations as in [49-timer.md](49-timer.md).

```bash
python3 '96-Local Testing/06-Miscellaneous/50-interactive_tester.py' --mode quick --seed 1             # PASS, 2 configurations, 3,041 checks, 8 exit scenarios, 3 piped games each
python3 '96-Local Testing/06-Miscellaneous/50-interactive_tester.py' --mode full --seed 1              # PASS, 3 configurations, 69,489 checks, 8 scenarios, 20 games, 4 assertion probes
CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/50-interactive_tester.py' --mode full --seed 2   # PASS, 3 configurations, 68,710 checks
python3 '96-Local Testing/06-Miscellaneous/50-interactive_tester.py' --mode stress --seed 3            # PASS, 3 configurations, 693,704 checks, 60 games
python3 '96-Local Testing/02-integration.py' --sanitizers                                              # PASS
python3 '96-Local Testing/03-consistency.py'                                                           # no errors
```

Warning-free with `-Wall -Wextra -Wconversion` under GCC 16.2 and 14.4 in a two-translation-unit build through `99-all.hpp`.

## Benchmarks

None: every operation is one formatted line or one extraction, and interactive problems are bound by the judge round trip.

## Sources

Catalog sweep by `@researcher` on 2026-10-08 (ledger in [00-sources.md](00-sources.md)); code written independently:

- CSES [3139](https://cses.fi/problemset/task/3139) and [3305](https://cses.fi/problemset/task/3305), AtCoder [practice_2](https://atcoder.jp/contests/language-test-202001/tasks/practice_2): `? …`/`! …`, `F i`/`S i` and `<`/`>` protocols motivate the free prefix argument and range expansion.
- Codeforces interactive-problem guide (blog 45307) and 1999G1 statement, seen only in search snippets (direct fetches returned 403): flush before reading, exit after `-1`, per-test budgets, verdict after the answer.

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): `readVerdict`, `readBool`/`readCmp`, comparator adapters, `readVec(n)`, a local interactor (that is `09-Contest Testing/05-interactor.py`).
- Buffered `03-fastio.hpp` readers are not used: bulk `fread` can block on a live pipe. `iostream` with the flush discipline above is the portable choice; `17-fast_io_advanced.hpp` must keep its interactive fallback.
- `08-Python` names its counterpart `ask`/`tell`/`finish`; the C++ names follow the inventory row (`ask`/`answer`).
- Exact GCC 14.2 and the Windows build were not run; `\r` stripping covers Windows line endings that reach the stream untranslated.

## Independent review

`@reviewer` (2026-10-08) confirmed one defect: an unsigned reply type let the judge's `-1` through as all-ones, and the evidence described that as a feature. Fixed by asserting a disabled sentinel for unsigned reply types, with two new assertion probes and an all-ones test; the suites were rerun in every mode. Nits taken: the complexity line now defines `L`, and one-byte reply types are documented.

## History

- 2026-10-08: created and verified (P229); reviewer finding on unsigned replies fixed the same day.
