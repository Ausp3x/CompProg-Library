# 03-fastio.hpp — evidence

`03-fastio.hpp` implements `FastInput<N>` and `FastOutput<N>` over a borrowed `FILE*` with 64 KiB default buffers: bytes, tokens, lines, decimal integers through 128 bits, exactly rounded doubles, fixed-precision double output, variadic `read`/`print`, EOF and explicit flush/error handling. Batch MI01, package P013; dependency P002 (template). Memory mapping, descriptor backends and SIMD parsing belong to `17-fast_io_advanced.hpp`.

## Contracts

### FastIoStatus

`Ok`, `Eof`, `Invalid`, `Overflow`, `Error`. Every read operation sets `FastInput::status`. Precedence when several apply to one token: `Error` (a `FILE` read error before the token boundary) over `Invalid` (syntax) over `Overflow` (out of range).

### FastInput

- Ownership: borrows a nonnull, open, byte-oriented `FILE*` (default `stdin`, asserted nonnull) that must outlive the object; never closes it, changes global stream settings or ties output. Copy and move are deleted. Construction and destruction do no I/O. Use one reader per stream; do not mix other readers or seek while it is active. Unused read-ahead is not returned to the stream: reposition explicitly before reusing the `FILE*`.
- Memory: `N` bytes (`N > 0`, compile-time) plus constant fields, besides the C library's buffer. Very large automatic objects can exceed the stack; allocate them appropriately. Time is linear in bytes consumed, malformed tokens included.
- `peek()`, `get()`: next unsigned byte `0..255` or `EOF`; only `get` consumes. NUL and high bytes are data. `space(c)` is exactly ASCII space, `\t`, `\n`, `\v`, `\f`, `\r`; `skipSpace()` returns whether a nonspace byte remains.
- `readChar(char &)`: skip whitespace, consume one byte. `readToken(string &)`: skip whitespace, read a maximal nonspace token, leave the delimiter unread; O(L) temporary storage.
- `readLine(string &)`: read bytes up to and excluding the next `\n`, consume the `\n`, and drop one trailing `\r` (CRLF input). Leading whitespace is kept and an empty line yields `""`. A final line without `\n` succeeds; at EOF with no bytes left it fails with `Eof`. After `readInt`/`readToken`, the rest of the current line, often empty, is the next line. Copies whole buffer chunks with `memchr`, O(L).
- `readInt(T &)`: every GNU integral type except `bool` (including `char`, wide and UTF character types and 128-bit integers) over its full range. The whole whitespace-delimited token must be `[+|-]digits`; leading zeros are allowed, signed `-0` is zero, and unsigned negative forms (including `-0`) are `Invalid`.
- `readDouble(double &)`: reads one token and parses it with `std::from_chars` (general format), which is correctly rounded (round to nearest) and locale independent. It accepts an optional leading `+` (but not `+-`), decimal and exponent forms such as `.5`, `5.`, `1E5` and `-.5e3`, and the spellings `inf`, `infinity` and `nan` in any case. Hexadecimal (`0x1p3`), a trailing `e` without digits, `,` decimal separators and any other trailing bytes are `Invalid`. Out-of-range magnitudes are `Overflow`, both above `DBL_MAX` and below half the smallest subnormal (for example `1e-400`), because that is libstdc++'s `result_out_of_range` behaviour. Only `double` is read: `float` callers convert, and exact `long double` parsing is not owned.
- `read(x)` and `read(xs...)`: dispatch by type, with `char` to `readChar`, `string` to `readToken`, `double` to `readDouble`, other integral types (including `int8_t`/`uint8_t` as numbers) to `readInt`, and any other range element-wise into its existing elements (size the container first). It returns false at the first failure, leaving later destinations unchanged and earlier ones assigned. `bool` and `float` destinations do not compile.
- Failure contract for all reads: the destination is unchanged, a bad token is consumed whole, and later reads continue after it. A complete token or line ending at EOF succeeds. A malformed one at EOF reports its failure, and the next read reports `Eof`. Bytes already returned by a partial `fread` stay readable, and the error is reported when they run out. Recovering from a file error requires a new reader.
- Interactive use: bulk `fread` may wait to fill the buffer on a live pipe. Use `FastInput<1>` for interactive protocols and flush prompts explicitly.

Correctness: the reader keeps the unread interval `[pos, len)` and refills only when it is empty; byte-element `fread` makes partial reads return complete bytes. `readInt` accumulates an unsigned magnitude and compares against `limit / 10` and `limit % 10` before each digit, so it never overflows; signed negatives build `-(v - 1) - 1`, keeping the minimum representable. After a syntax or overflow failure the parser advances to the token boundary without assigning. `readLine` appends `[pos, newline)` chunks found by `memchr`, consuming and refilling whole buffers until a newline (consumed) or EOF/error from `peek`; status is checked before assigning. `readDouble` delegates to `from_chars` over the exact token bytes (correct rounding is the standard's requirement; libstdc++ uses fast_float with an exact fallback), and `ptr == end` rejects trailing garbage.

### FastOutput

- Ownership mirrors `FastInput` (default `stdout`). Memory `N` bytes. `put`, `write` and `writeInt` return whether the bytes were buffered or transferred without a detected error. The view passed to `write` must not overlap the writer's buffer.
- `write(string_view s, char end = '\0')`, `writeInt(T x, char end = '\0')`, `writeDouble(double x, int p, char end = '\0')`: a nonzero `end` byte is appended after the value. Use `put('\0')` for a real NUL.
- `writeDouble`: `std::to_chars(..., chars_format::fixed, p)` with `0 <= p <= 200` (asserted). The output is the exact decimal expansion of the binary value, correctly rounded to `p` digits (ties to even on the exact value), identical to glibc `printf("%.*f")`. Negative zero and small negatives that round to zero print `-0.000`. Infinities print `inf` and `-inf`, NaNs `nan` and `-nan`. Every result fits a 512-byte stack buffer (309 integer digits, sign, point and 200 decimals). `float` and integer arguments convert to `double`. A `long double` argument is a compile error rather than a silent narrowing.
- `writeValue(x)`: `char` to `put`, anything convertible to `string_view` to `write`, `float`/`double` to `writeDouble(x, precision)`, other integral types to `writeInt`, and ranges element-wise separated by single spaces (nested ranges flatten). `bool` does not compile.
- `print(xs...)`: `writeValue` each argument, separated by single spaces, then `\n`. `print()` writes only `\n`. An empty range contributes nothing between its separators.
- `precision` (public field, default 15): the decimals used by `writeValue`/`print` for floating values.
- `drain()` moves the buffer into the C stream (which may still buffer it). `flush()` drains and calls `fflush`; call it to observe deferred errors and before every interactive read. The destructor best-effort flushes pending output, including bytes drained earlier, but cannot report failure.
- Errors are sticky: after a short or failed `fwrite` or a failed `fflush`, all later writes and flushes fail without retry; bytes already transferred cannot be rolled back. Recovering requires a new writer.

Correctness: `writeInt` takes the magnitude by unsigned subtraction (defined for the signed minimum) and emits digits in reverse. `writeDouble` uses `to_chars` with explicit precision, specified as equivalent to `%.*f` in the "C" locale and implemented exactly by libstdc++ with Ryu printf. Buffer transfers keep byte order, a short transfer is never reported as success, and `flush` includes errors the C stream defers.

```cpp
FastInput<> in; FastOutput<> out;
int n; in.read(n); vector<lng> a(n); in.read(a);
out.print(n, a); out.writeDouble(3.14159, 3, '\n');
bool ok = out.flush();
```

## Feature-to-test map

| Operation | Independent coverage |
|---|---|
| `readInt`/`writeInt`, all integer widths | `std::to_chars` expected bytes; exhaustive 8-bit (quick) and 16-bit (full) domains; random 32/64/128-bit; `char`, wide/UTF code-unit and `long long` identities; Python arbitrary-precision 128-bit round trip (3,003 pairs full, 30,003 stress) |
| Integer syntax/overflow/recovery | Bare/repeated signs, punctuation, hex-like, embedded NUL, byte 255, boundary ±1, 1000-digit tokens, unsigned `-0`; delimiter- and EOF-terminated failures with destination preservation |
| `peek`/`get`/`space`/`skipSpace`/`readChar`/`readToken` | All 256 bytes raw, buffers 1/7/65536, empty and whitespace-only input, NUL/high-byte tokens, delimiter preservation, EOF preservation; 10 KB/1 MiB/8 MiB binary round trips |
| `readLine` | Independent C++ splitter over fixtures (empty, `\n`, `\n\n`, CRLF, lone `\r`, `\r\r\n`, NUL, leading/trailing spaces, 70,000-byte line, lines straddling 65,536) and random `ab\r\n \t` strings at buffers 1/7/65536; interleaving with `readInt`/`readToken`; Python `bytes.split` oracle over random bytes including NUL and 255 (60,005 bytes full, 600,005 stress); EOF and fault-injected `Error` preserve the destination |
| `readDouble` | `strtod` bit-exact comparison for `%.17g`, `%.3e`, `%.25f` forms of random finite bit patterns plus fixed spellings (`-0`, `+0`, `.5`, `5.`, `1E5`, subnormals, `DBL_MAX`, 30-digit integers, `inf`, `nan`, `Infinity`); Python `float()` correctly-rounded oracle on 3,000 (full) / 30,000 (stress) random decimal tokens with exponents to 330 and signs, including overflow and total underflow (`Overflow`); invalid forms (`1.5e`, `0x10`, `+`, `+-1`, `--1`, `.`, `1,5`, `e5`, `1e400x`) at a delimiter and at EOF, with recovery; fault-injected `Error` preserves the destination |
| `writeDouble` | glibc `printf("%.*f")` byte comparison for fixed values (zeros, halves, `DBL_MAX`, `DBL_MIN`, `5e-324`, infinities, `999.9995`) and random finite values at precisions 0–20 and 200, with and without `end`; NaN spelling; Python `format(x, '.{p}f')` exact oracle on 3,000 (full) / 30,000 (stress) random bit patterns |
| `read`/`print`/`writeValue`/`write` end/`precision` | Mixed `int`, `string`, `char`, `double`, `uint8_t` read; range read stopping at EOF with partial assignment; `print` with string literal, `string`, `char`, `double` at precision 3, vectors, empty vector, nested vectors, `lll`, `int8_t`; `print()`; `write(s, '!')`; `float` through `writeValue`; compile-time checks that `writeDouble` rejects `long double` and accepts `double`, `float` and `int` |
| Flush, lifetime, errors | `tmpfile` reuse; descriptor observation of destructor flush after `drain`; GNU `fopencookie` short/failed reads and writes, error precedence for tokens, integers, lines and doubles, delayed `fflush` failure, sticky failure without retries, empty lifetime without I/O; pipe tokens with `N = 1` before the writer closes |
| Preconditions | Four checked assertion probes: null input, null output, precision -1, precision 201; deleted copy/move and positive `N` at compile time |

## Commands and results

P013 package run, 2026-10-07, GCC 16.2.1, GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan/UBSan `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all` with leak detection. The fastio full run passed all 3 configurations and 4 assertion probes.

```sh
python3 '96-Local Testing/01-run.py' --mode quick --filter 06-Miscellaneous --no-integration                            # PASS (all 14 suites)
for s in 01-random 02-customhash 03-fastio 04-compression 05-binarysearch 06-bit_operations 07-permutation; do
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode full --seed 20260927                                # PASS x7, 3 configurations each
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode stress --seed 20260928 --configuration optimized   # PASS x7
done
python3 '96-Local Testing/06-Miscellaneous/03-fastio_benchmark.py'                                                       # PASS, record rewritten
python3 '96-Local Testing/02-integration.py'                                                                             # PASS (every header alone, Basic/All aggregates)
python3 '96-Local Testing/03-consistency.py'                                                                             # no errors
```

## Benchmarks

The [benchmark script](<../../96-Local Testing/06-Miscellaneous/03-fastio_benchmark.py>) with its `.cpp` and `.json` record times complete pipelines on a warmed regular file: construct buffers, parse all input, transform, format, flush. Every output byte is compared outside the timed region with independently generated expectations (`to_chars` for integers, `snprintf("%.9f")` for doubles). One warmup and five repetitions with rotated method order. Recorded 2026-10-07 on Intel Core i9-11900H, GCC 16.2.1, `-std=gnu++20 -O2 -DNDEBUG`, seed 20260927. Medians in milliseconds:

| Values / workload | Fast 4096 | Fast 65536 | fscanf/fprintf |
|---|---:|---:|---:|
| 32 small integers | 0.0036 | 0.0035 | 0.0062 |
| 32 full-width integers, mixed whitespace | 0.0050 | 0.0049 | 0.0079 |
| 32 doubles (`%.17g` in, `%.9f` out) | 0.0073 | 0.0075 | 0.0164 |
| 20,000 small integers | 1.075 | 1.050 | 2.430 |
| 20,000 full-width integers | 2.374 | 2.259 | 4.391 |
| 20,000 doubles | 4.170 | 4.123 | 12.465 |
| 500,000 small integers | 31.15 | 29.85 | 70.42 |
| 500,000 full-width integers | 55.20 | 53.69 | 107.00 |
| 500,000 doubles | 100.48 | 96.37 | 291.31 |

These are shared-host observations, not timing gates. They support the scalar buffered design and the 64 KiB default. Doubles take `to_chars`/`from_chars` at about three times the speed of `fscanf`/`fprintf`. Cold disk, interactive, malformed-input and network throughput were not measured.

No Barrett/Montgomery backend is involved.

## Sources

- [OI Wiki, 输入输出优化](https://oi-wiki.org/contest/io/): buffered `fread`/`fwrite`, signed-minimum handling, interactive flushing (read 2026-09-27).
- cppreference [fread](https://en.cppreference.com/w/c/io/fread.html), [fwrite](https://en.cppreference.com/w/c/io/fwrite.html), [fflush](https://en.cppreference.com/w/c/io/fflush.html): short transfers, EOF versus `ferror`, delayed failure (2026-09-27).
- Completeness sweep 2026-10-07 (`@researcher`): [KACTL FastInput.h](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/various/FastInput.h), [Nyaan fastio](https://raw.githubusercontent.com/NyaanNyaan/library/master/misc/fastio.hpp), [maspypy io](https://raw.githubusercontent.com/maspypy/library/main/other/io.hpp), [yosupo fastio](https://raw.githubusercontent.com/yosupo06/yosupo-library/main/src/yosupo/fastio.hpp), [ei1333 scanner](https://ei1333.github.io/library/other/scanner.hpp) and [printer](https://ei1333.github.io/library/other/printer.hpp), [hitonanode reader](https://hitonanode.github.io/cplib-cpp/utilities/reader.hpp). Variadic `read`/`print`, range reads and writes, and `end` bytes come from these. Double conventions: maspypy and yosupo parse with `stod` (locale-dependent), maspypy prints with `%.15f`. This header uses `from_chars`/`to_chars` instead, which are exact and locale-free; the default `precision = 15` follows maspypy.
- Standard: `std::from_chars` and `std::to_chars` for floating point ([charconv] in C++17/20), with correct-rounding and `printf`-equivalence requirements as summarized on [cppreference from_chars](https://en.cppreference.com/w/cpp/utility/from_chars) and [to_chars](https://en.cppreference.com/w/cpp/utility/to_chars).

## Limits and handoffs

- `readDouble` reports total underflow as `Overflow`. Exact `long double` I/O is not owned. Locale-specific formats are not supported.
- Floating `from_chars`/`to_chars` have been in libstdc++ since GCC 11; `double` parsing uses the portable fast_float path since GCC 12. The Codeforces MinGW GCC 14.2 floor was not built locally, so portability there rests on that library history, not on a test.
- Advanced backends (`mmap`, raw `read`/`write`, SIMD digit parsing) remain with `17-fast_io_advanced.hpp`.
- No online submission was made and no judge acceptance is claimed.

## History

- 2026-09-27: P013 first verification, full and stress suites and benchmark passed.
- 2026-10-07: P013 re-audit, finding 1 (missing `readLine`, `readDouble`, `writeDouble`) fixed, `read`/`print`/`writeValue`/`end`/`precision` added, contracts moved out of code comments, brace style normalized; full, stress, benchmark and integration passed; `@reviewer` (2026-10-08) found no defects.
