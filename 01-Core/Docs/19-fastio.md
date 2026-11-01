# 19-fastio.hpp — evidence

`19-fastio.hpp` implements `FastInput<N>` and `FastOutput<N>` over a borrowed `FILE*`: bytes, tokens (zero-copy views), lines, integers through 128 bits in bases 2–36, exactly rounded `float`/`double`, text-number hooks (`InfInt`, `InfIntMini`, `Rational`), variadic `read`/`print` over ranges, pairs and tuples, EOF and explicit flush/error handling. Package P018 (stage 4), a Core full type since 2026-11-01: moved from `06-Miscellaneous/03-fastio.hpp` (P013) with AVX2 kernels and Linux mapping, absorbing the planned `06-Miscellaneous/17-fast_io_advanced.hpp` (P066), so the Library Checker big-integer drivers need no private I/O header (user decisions 2026-11-01, [10-decisions.md](../../00-Guidelines/10-decisions.md)). Dependency: `01-template.hpp` only; the hooks are concepts, so the header includes no other Core type.

## Contracts

### FastIoStatus

`Ok`, `Eof`, `Invalid`, `Overflow`, `Error`. Every read operation sets `FastInput::status`. Precedence when several apply to one token: `Error` (a `FILE` read error before the token boundary) over `Invalid` (syntax) over `Overflow` (out of range).

### FastInput

- Template: `N = 0` reads the whole stream at construction: a private read-only mapping of a regular file from its current offset under `#ifdef __linux__` (`fstat`, `mmap` with `MAP_POPULATE`), else every byte `fread` returns into a vector; the destructor unmaps. `N = 1` reads one byte per refill, for interactive protocols (bulk `fread` may wait on a live pipe; flush prompts explicitly). `N > 1` is a buffer of `N` bytes (default 65,536). Memory: `N` bytes plus a copy of a token that crosses a refill, or for `N = 0` the stream (the mapping, or the vector, which may reach twice the stream).
- Ownership: borrows a nonnull, open, byte-oriented `FILE*` (default `stdin`, asserted) that must outlive the object; never closes it, changes global stream settings or ties output. Copy and move are deleted. For `N > 0` construction and destruction do no I/O. Use one reader per stream; do not mix other readers or seek while it is active. Unused read-ahead is not returned to the stream.
- `peek()`, `get()`: next unsigned byte `0..255` or `EOF`; only `get` consumes. NUL and high bytes are data. `space(c)` is exactly ASCII space, `\t`, `\n`, `\v`, `\f`, `\r`; `skipSpace()` returns whether a nonspace byte remains.
- `readChar(char &)`: skip whitespace, consume one byte. `readToken(string_view &)`: skip whitespace, return a maximal nonspace token, leave the delimiter unread. The view points into the input (the buffer, the mapping or the vector) unless the token crossed a refill, when it points into the reader's token copy; for `N > 0` it stays valid until the next read operation, for `N = 0` for the reader's lifetime. `readToken(string &)` copies the view.
- `readLine(string &)`: bytes up to and excluding the next `\n`, which is consumed, with one trailing `\r` dropped (CRLF input). Leading whitespace is kept and an empty line yields `""`. A final line without `\n` succeeds; at EOF with no bytes left it fails with `Eof`. After a token read, the rest of the current line, often empty, is the next line.
- `readInt<B>(T &)`: every GNU integral type except `bool` (including `char`, wide and UTF character types and 128-bit integers) over its full range, in base `B` (a template argument in `[2, 36]`, statically asserted; letters in either case). The whole token must be `[+|-]digits`; leading zeros are allowed, signed `-0` is zero, and unsigned negative forms (including `-0`) are `Invalid`.
- `parseInt<B>(string_view s, T &)`: the value of `s` under `readInt`'s rules. Precondition: `s` is a view this reader's `readToken` returned and that is still valid (the kernels load the 32 bytes before its end from the buffer holding it, or from a zero-padded copy near the buffer's start). It lets a caller test a token's length before choosing a type, as the Library Checker drivers do.
- `readDouble(T &)` for `T` = `float` or `double`: one token through `std::from_chars` (general format, correctly rounded to `T`, locale independent). An optional leading `+` (not `+-`), decimal and exponent forms such as `.5`, `5.`, `1E5`, `-.5e3`, and `inf`, `infinity`, `nan` in any case. Hexadecimal (`0x1p3`), a trailing `e` without digits, `,` separators and trailing bytes are `Invalid`; magnitudes out of range are `Overflow`, both above the maximum and below half the smallest subnormal (`1e-400` for `double`), as libstdc++'s `result_out_of_range` reports them. `long double` is not owned.
- `readText<B>(T &)` for a type with `bool read(string_view, uint base)` (`InfInt`, `InfIntMini`, `Rational`): the token through `x.read(s, B)`, `Invalid` when it returns false (the type's contract says what `x` holds then; all three leave it unchanged). A type that takes `lng` exactly (`T{v}` without narrowing) gets decimal tokens of at most 18 digits and hex tokens of at most 15 digits by assignment instead, and one that takes `lll` exactly also 19–38 decimal and 16–31 hex digits; such a type's `read` must agree with assignment on those tokens.
- `read<B>(x)` and `read<B>(xs...)`: by type, `char` to `readChar`, `string` and `string_view` to `readToken`, `float` and `double` to `readDouble`, other integral types (including `int8_t`/`uint8_t`, as numbers) to `readInt<B>`, text-number types to `readText<B>`, a range element by element into its existing elements (size it first; nested ranges recurse), and a pair, tuple or other tuple-like type element by element. It stops at the first failure: later destinations and elements are unchanged, earlier ones assigned. `bool` destinations do not compile.
- Failure contract for all reads: the destination is unchanged, a bad token is consumed whole, and later reads continue after it. A complete token or line ending at EOF succeeds; a malformed one at EOF reports its failure and the next read reports `Eof`. Bytes already returned by a partial `fread` stay readable, and the error is reported when they run out; a token or line that the error cuts (no delimiter between it and the error) reports `Error` for every `N` (for `N = 0` the load holds back the trailing nonspace bytes before the error, past the 32-byte token path, until a read reaches them). Recovering from a file error requires a new reader.

### FastOutput

- `N > 0` bytes of buffer (statically asserted; default 65,536). Ownership mirrors `FastInput` (default `stdout`). `put`, `write`, `writeInt`, `writeDouble`, `writeText`, `writeValue` and `print` return whether the bytes were buffered or transferred without a detected error. A view passed to `write` must not overlap the buffer; a view of `N` bytes or more is drained past the buffer by one `fwrite`.
- `write(string_view s, char end = '\0')`, `writeInt<B>(T x, char end = '\0')`, `writeDouble(double x, int p, char end = '\0')`: a nonzero `end` byte follows the value; use `put('\0')` for a real NUL. `writeInt` writes base `B` in `[2, 36]` (statically asserted) with uppercase letters and `-` for negatives; it needs 160 bytes of room, which a buffer below 160 bytes takes through a stack copy.
- `writeDouble`: `std::to_chars(..., chars_format::fixed, p)` with `0 <= p <= 200` (asserted): the exact decimal expansion correctly rounded to `p` digits (ties to even on the exact value), identical to glibc `printf("%.*f")`. Negative zero and small negatives that round to zero print `-0.000`; infinities `inf`/`-inf`, NaNs `nan`/`-nan`. Every result fits a 512-byte stack buffer (309 integer digits, sign, point and 200 decimals). `float` and integer arguments convert to `double`; `long double` is a compile error rather than a silent narrowing.
- `writeText<B>(x)` for a type with `charsBound(base)` and `toChars(first, last, base)` (`InfInt`): `toChars` straight into the free buffer; when it refuses (it writes nothing then), the text goes after a drain, or through a temporary when `charsBound` exceeds `N` (2 MiB-aligned and advised as huge pages under Linux from 512 KiB). The hook contract: on success `toChars` writes `[first, ptr)` and may overwrite bytes in `[ptr, last)`; on failure it writes nothing.
- `writeValue<B>(x)`: `char` to `put`, `bool` as `0`/`1`, anything convertible to `string_view` to `write`, floating values to `writeDouble(x, precision)`, other integral types to `writeInt<B>`, text-number types to `writeText<B>`, ranges element by element separated by single spaces (nested ranges flatten), and tuple-like values the same way.
- `print<B>(xs...)`: `writeValue<B>` each argument, separated by single spaces, then `\n`; `print()` writes only `\n`. An empty range or tuple contributes nothing between its separators. Integers alone (not `char`) take one room check for the whole line.
- `precision` (public field, default 15): the decimals `writeValue`/`print` use for floating values.
- `drain()` moves the buffer into the C stream (which may still buffer it). `flush()` drains and calls `fflush`; call it to observe deferred errors and before every interactive read. The destructor flushes pending output, including bytes drained or written past the buffer earlier, best effort, and cannot report failure.
- Errors are sticky: after a short or failed `fwrite` or a failed `fflush`, all later writes and flushes fail without retry; bytes already transferred cannot be rolled back. Recovering requires a new writer.

```cpp
FastInput<> in; FastOutput<> out;
int n; in.read(n); vector<pair<int, lng>> a(n); in.read(a);
out.print(n, a); out.writeDouble(3.14159, 3, '\n');
iint x; in.read<16>(x); out.print<16>(x * x);
bool ok = out.flush();
```

### fastio_detail

Not public API. Input: `decWord`, `dec8` (SWAR digit test and value of eight bytes), `dec19` (1–19 decimal digits ending at a 32-byte window, AVX2 blend with the `TAIL` mask and two multiply-add steps, or three SWAR groups), `hexWord`, `hex8`, `hex16` (1–16 hex digits, AVX2 nibble table or SWAR), `hex32` (1–32 hex digits into two words); output: `decLen` (digit count by `bit_width * 1233 >> 12` and one power), `dec8Chars` (eight digits by reciprocal products in lanes), `dec16Chars` (AVX2, sixteen digits), `writeDec` (64-bit: two digits by table, eight by `dec8Chars`, twenty by a shifted store of the top group and the low sixteen; 128-bit: one Möller–Granlund 2-by-1 step at 10^19 and `dec19Chars`), `hex16Chars`, `writeHex` (64- and 128-bit by shifted 16-digit stores), `writeBase` (other bases, digit by digit, out of line); concepts `Int` (integer tokens, 128-bit by name), `Exact`, `TextIn`, `TextOut`; `HUGE_PAGE` and `AlignedFree` for the long-text temporary. The kernels write up to 24 (64-bit) or 40 (128-bit) bytes past the start of their text, inside `writeInt`'s 160-byte room. They repeat InfInt's short-token kernels on purpose: the header includes no Core type and InfInt includes no I/O header, so each copies the few lines it needs and the hooks stay generic.

### Correctness argument

- Token scan: the AVX2 class test looks each byte up by its low nibble in a 16-entry table holding `' '` at 0 and `\t`–`\r` at 9–13 and compares the entry with the byte; a byte matches exactly when it is one of the six (`pshufb` returns 0 for bytes from `0x80`, and 0 never equals its own entry `' '`). `readToken`'s fast path needs 32 readable bytes from `p` (`e - p >= 32`): the token starts after the trailing ones of the whitespace mask, and ends at the first set bit of the mask with those ones cleared, or past the load through `tokenEnd`. For `N > 0` a token reaching the end of the block takes the out-of-line path, which copies it across refills. The scalar scan flags bytes below `'!'` eight at a time by `(x - 0x21…) & ~x & 0x80…`, whose lowest flag is exact (borrows only reach higher bytes), and finishes byte by byte, so control bytes that are data resume the scan. A software prefetch 1 KiB ahead uses an integer address, which never faults past the buffer.
- Integer windows: the kernels read the 32 bytes ending at the token's end; `parseInt` takes them in place only when the buffer holding the token has 32 bytes before that end (64 for two decimal windows), else from `pad`, the token right-aligned in `'0'` bytes. Digits are validated in the same pass (a nondigit fails the kernel, and the out-of-line path then reports `Invalid` or the digit-by-digit result), magnitudes are compared with the type's limit plus one for negatives, and the sign is applied as `(u ^ m) - m`, defined for the minimum. Tokens of more digits than a window go digit by digit against `limit / B` and `limit % B`, and after an overflow only validity is checked, eight bytes at a time in bases 10 and 16, so `Invalid` outranks `Overflow`.
- Output: `writeDec` stores whole groups and returns the length from the digit count, so bytes past the end are scratch inside the room it checked; `writeDec(ulll)` takes the quotient by 10^19 from the reciprocal `⌊(2^128 - 1) / 10^19⌋ - 2^64` with one masked and one rare correction, exact for every `hi < 10^19`. The base-`B` paths compute digits from the unsigned magnitude `(v ^ m) - m`.
- Exhaustive and differential checks of each claim: [Feature-to-test map](#feature-to-test-map).

### Optimization scope

Core full profile: AVX2 kernels behind `#ifdef __AVX2__` with SWAR or scalar fallbacks giving identical results (tested in both builds), no BMI2 or FMA requirement, the Linux mapping behind `#ifdef __linux__` with an identical `fread` path ([03-cpp.md](../../00-Guidelines/03-cpp.md), Toolchain and judges). The base is a template argument and the common paths are inlined with their rare branches out of line (`tokenRest`, `intRest`, `textRest`, `padded`, `writeBase`), each choice measured in [Benchmarks](#benchmarks).

### Legacy mapping

| Old name or form | Replacement |
|---|---|
| `06-Miscellaneous/03-fastio.hpp` (`FastInput<N>`, `FastOutput<N>`, `FastIoStatus`) | the same names in `01-Core/19-fastio.hpp`; no forwarding header |
| runtime `base` member (stage-4 development form) | the template argument `<B>` of `readInt`, `parseInt`, `readText`, `read`, `writeInt`, `writeText`, `writeValue`, `print` |
| planned `MmapInput` (06 row 17) | `FastInput<0>` |
| planned `SyscallOutput` (06 row 17) | `FastOutput` (one `fwrite` per block or long view) |
| `00-bigio.hpp` (stage-4 driver header, retired): `readAll`, `next`, `text`, `parseDec`, `parseHex`, `skipSpace` | `FastInput<0>`, `read(string_view &)`, the view itself, `parseInt`, `parseInt<16>`, `skipSpace` |
| `00-bigio.hpp`: `hugeMap`, `writeHex`, `writeAll`, `to_chars` into the map | `FastOutput` with `print`, `print<16>`, `writeInt` |

## Feature-to-test map

[`19-fastio_tester.py`](<../../96-Local Testing/01-Core/19-fastio_tester.py>) builds optimized scalar, checked scalar and optimized AVX2 configurations of [`19-fastio_tester.cpp`](<../../96-Local Testing/01-Core/19-fastio_tester.cpp>) in six translation units (full and stress add ASan/UBSan scalar and AVX2); each phase prints a digest that must agree across configurations, so the AVX2 kernels and the scalar fallbacks give identical results. Quick limits the type matrices to bases 2, 7, 10, 16, 36 and buffers 0, 7, 64, 160. The one-unit build (no `FASTIO_PART`), which `02-integration.py --sanitizers` compiles like every Core tester, runs a smoke subset (whitespace class, kernels, token grid and streams, print forms, bytes, lines, variadic reads, faults, lifetime, write forms) to fit that pass's 1664 MB and 180 s per compile (measured 1010 MB, 30 s at `-O1 -g` with ASan/UBSan); the whole suite under ASan/UBSan is the tester's own full and stress runs (user decision 2026-11-01).

| Operation | Coverage | Oracle |
|---|---|---|
| `FastIoStatus` precedence | `Error` over `Invalid` and `Overflow` through `fopencookie` faults at `N` = 0, 1, 7, 65536, including tokens and lines the error cuts; `Invalid` over `Overflow` (overflowing digits then a bad byte, 1000-digit tokens) | model of the contract; digit-loop parse |
| `FastInput<0>` | the mapping of a regular file from offset 0 and from nonzero offsets (`fseek`, consumed bytes), view addresses, unmapping; the `fread` path over a pipe and `fmemopen`; empty input, end of file; the protocols over a file and a pipe | byte model; the mapped flag |
| `N` = 1 and `N` > 1 | an interactive pipe in two batches; buffers 1, 2, 3, 5, 7, 31, 32, 33, 64, 65536 | byte model |
| `space`, `peek`, `get`, `skipSpace`, `readChar` | all 256 bytes; random operation mixes over streams up to 2 MB | the six-byte class; a model cursor |
| `readToken` (`string`, `string_view`) | whitespace 0–70 by token length 0–200; views into the buffer, the token copy or the mapping, checked until the reader ends | model substring and address |
| `readLine` | CRLF, NUL, 0xFF, lines over 65,536 bytes, a line after tokens | C++ splitter; Python `bytes.split` |
| `readInt`, `parseInt` | 17 types by bases 2–36; lengths 1–40, 63–65, 300, 1000; signs, `-0`, leading zeros, limits ±1, every nonspace byte, `Invalid` over `Overflow`; 16-bit types exhaustive; tokens near the buffer start; a failed parse of a view (`Overflow`, or `Invalid` for a sign of an unsigned type) and then a successful one of the same view reporting `Ok`, inline and out of line, `N` = 65536, 64, 1, 0, bases 10 and 16 | `from_chars` and a digit loop, each checked against the other; Python integers (the 128-bit decimal round trip) |
| Kernels (`dec19`, `hex16`, `hex32`, `decWord`, `hexWord`, `dec8`, `hex8`, `spaceMask`, `tokenEnd`) | every length, position and byte value | digit loop; equal scalar and AVX2 digests |
| `readDouble` (`float`, `double`) | rounding midpoints and neighbours, subnormals, overflow, special values, hex, `+-` and trailing bytes | `strtof`/`strtod`; Python `float()` for `double` (a Python `float32` conversion rounds twice, so `float` uses `strtof` only) |
| `readText<B>` | InfInt in bases 2–36, InfIntMini, `Rational` over `lng`, `lll` and InfInt; the assignment shortcut at lengths 1–40 in bases 10 and 16 and near the buffer start; a probe type | the type's `read`; Python integers for InfInt |
| `read<B>` | variadic, ranges, nested ranges, pair, tuple, empty tuple, partial assignment on failure | fixed expectations; digit-loop parse |
| `put`, `write` | the `end` byte; views of 0, 1, N − 1, N, N + 1 and 3N + 2 bytes; a real NUL | built string |
| `writeInt<B>` | every type, bases 2–36, uppercase, buffers 1, 64, 159–4096 at every fill level | `to_chars` and a digit loop; Python |
| `writeDouble` | every precision 0–200, `inf`, `nan`/`-nan`, `-0` and small negatives, `float` and integer arguments | glibc `printf`; Python `format` |
| `writeText<B>` | straight into the buffer, after a drain, through the temporary (524,286 to 3 MiB digits) | the source digits; `toString(16)` against a word dump; Python |
| `writeValue`, `print` | `bool` as 0/1, `vector<bool>`, ranges, tuples, `char` (on the integer path and the mixed one), `print()` | fixed and built strings |
| `precision`, `drain`, `flush`, `error`, destructor | precision kept; sticky errors without retry; `fflush` failure; the destructor's flush of pending and drained bytes; no I/O at construction | cookie byte counts; `pread` |
| Preconditions | null `FILE*` (input `N` > 0 and `N` = 0, output), precision −1, 201 and 201 through the field | `SIGABRT` with the assertion message |
| Compile rejections | `bool` and `long double` reads, `long double` writes, bases 1 and 37, `writeInt(bool)`, `FastOutput<0>`, `FastInput<-1>`, copy and move | the expected compiler diagnostic |

Every check and the four Python oracles of the 06 tester have an equal or stronger counterpart here, with `N = 0` added.

## Commands and results

P018 stage 4, the `/finish` runs of 2026-11-02 (Opus 5.5 at xhigh): GCC 16.2.1 20260810 and GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14.8, Intel Core i9-11900H (governor powersave, EPP performance, turbo on, THP always; desktop applications and a VM ran throughout, and each suite ran beside the InfInt suite of its mode). Header `9202a3e985b6ad53`, tester `d5f7a8e5e8af050a`, InfInt `8eb193c5b3ab3e93`; the drivers and the judge reproduction ran alone after the suites. The benchmark ran on 2026-11-01 on header `1b721b07bb1ffcb6`, which differs from the final one by the `/finish` renames (`dec8`, `dec8Chars`, `dec16Chars`) alone, with identical machine code ([07-infint.md](07-infint.md#commands-and-results)).

```bash
python3 '96-Local Testing/01-Core/19-fastio_tester.py' --mode quick --seed 241                 # PASS, 3 configurations, 33 sections with equal digests, 34,918 Python-oracle protocol cases each, 6 precondition and 11 compile-rejection probes, 222 s, MEMORY peak=2901MB cap=4096MB oom_kills=0
python3 '96-Local Testing/01-Core/19-fastio_tester.py' --mode full --seed 242                  # PASS, 5 configurations, 63 sections with equal digests, 345,649 protocol cases each, 1424 s, MEMORY peak=3566MB cap=4096MB oom_kills=0
CXX=g++-14 python3 '96-Local Testing/01-Core/19-fastio_tester.py' --mode full --seed 243       # PASS, 5 configurations, 63 sections with equal digests, 344,500 protocol cases each, 1373 s, MEMORY peak=3553MB cap=4096MB oom_kills=0
python3 '96-Local Testing/01-Core/19-fastio_tester.py' --mode stress --seed 244 --rounds 1     # PASS, 5 configurations, 63 sections with digests equal across them, 1,722,720 protocol cases each, 1588 s, MEMORY peak=3564MB cap=4096MB oom_kills=0
python3 '96-Local Testing/02-integration.py' --sanitizers                                       # PASS, 183 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizer self-tests (this tester's one-unit smoke build among them), about 125 s, MEMORY peak=3896MB cap=8192MB oom_kills=0
python3 '96-Local Testing/03-consistency.py'                                                    # no errors; --braces on the header and the tester: no violations
g++ -O2 -std=c++23 -DEVAL -DONLINE_JUDGE [-march=native] -Wall -Wextra -Werror [-fsanitize=address,undefined] <driver>.cpp   # the six drivers (final headers) built three ways (AVX2 and scalar with ASan/UBSan, scalar optimized; `-Werror`) on all 141 regenerated judge cases: 423 runs, output identical to the reference answers, no sanitizer report, 162 s, MEMORY peak=2917MB
python3 '96-Local Testing/00-Tools/07-judge.py' reproduce <problem> '97-Online Testing/src/01-Yosupo/02-Big Integers/<driver>.cpp' --label final-20261102 --runs 3   # the six problems, every case AC (max / sum ms): addition 158.7 / 1453.2, multiplication 260.5 / 2806.0, division 217.6 / 2009.3, hex addition 5.0 / 53.3, hex multiplication 14.9 / 200.9, hex division 20.2 / 213.8 (final headers, idle host after the suites; host drift and the leaders: 07-infint.md, Library Checker reproduction)
python3 '96-Local Testing/01-Core/19-fastio_benchmark.py'                                       # PASS (header 1b721b07bb1ffcb6), every output compared; table below, 7 s, MEMORY peak=325MB
```

Read-error probes (2026-11-01 on header `1b721b07bb1ffcb6`; scratchpad, not retained): `fopencookie` streams that fail after their bytes, with cut tails of 0, 1, 31, 32 and 100 bytes behind empty, short, 200-byte and line heads, through token, byte, peek-then-token and line reads at `N` = 0, 7 and 65536: every case reports `Error` and no cut token is accepted, AVX2 and scalar, ASan/UBSan, `-Wall -Wextra -Wshadow -Wconversion -Werror`.

## Benchmarks

### Library Checker drivers against the retired private header

Drivers on header `b7b315c8603e0c0b` (the final header `9202a3e985b6ad53` differs by the review's `parseInt` status store, the `HUGE_PAGE` rename, comments and the `/finish` renames, which compile to identical machine code ([07-infint.md](07-infint.md#commands-and-results)); with InfInt's C15 it runs the same instructions on every case, medians 1.0000, and cycles within the A/A spread: Design measurements) against `B8`, the retired `00-bigio.hpp` drivers on the InfInt of the same session, every judge case below 60 ms, `perf stat` cycles:u medians of interleaved runs pinned to one core (15 runs warm; 9 cold, a 96 MB thrash before each run), 2026-11-01:

| Problem | Cases | Warm sum | Warm median | Cold sum | Cold median |
|---|---:|---:|---:|---:|---:|
| addition | 10 | 0.941 | 0.971 | 0.947 | 0.974 |
| multiplication | 6 | 0.957 | 0.982 | 0.959 | 0.982 |
| division | 13 | 0.918 | 0.996 | 0.905 | 0.998 |
| hex addition | 24 | 0.956 | 0.964 | 0.942 | 0.955 |
| hex multiplication | 22 | 0.988 | 0.986 | 0.988 | 0.988 |
| hex division | 27 | 0.996 | 0.986 | 0.998 | 0.985 |

The cases above 1.01 in either pass (the 2M-cycle `example_00` files, hex division `burnikel_ziegler_bound_02` at 1.277, `length_ratio_integer_00`, `max_00`, `max_01`, `a_max_b_random_01`/`02`, `burnikel_ziegler_bound_03`, hex multiplication `max_max_00`) read 0.965–1.013 in a 21-run recheck, each with fewer instructions than `B8` (0.981–0.999). Against the morning's stage-4 drivers (`00-bigio.hpp` on the earlier stage-4 InfInt), every case including the large ones, 5 runs: per-problem cycle sums 0.986, 0.990, 0.979, 0.951, 0.989 and 0.981 in the order above, the cases above 1.02 at 0.942–1.012 in a 21-run recheck.

Output path: the hex division driver with `FastOutput` against the whole output in one 2 MiB-aligned, huge-page-advised mapping and one `write` (the retired header's form), median cycles of 21 runs: `max_00` 4.776M against 4.762M, `large_00` 4.773M against 4.726M (within 1%; the variant also reads every token through InfInt, so its small and medium cases are not comparable). `FastOutput` stays.

### Design measurements

Judge flags (`g++ -O2 -std=c++23 -DEVAL -DONLINE_JUDGE -march=native`, GCC 16.2 unless stated, Intel Core i9-11900H), `perf stat` cycles:u and instructions:u, medians of interleaved runs pinned to one core, 2026-11-01; probes and switch headers in the scratchpad, not retained. Replicated inputs: `cc200` is 200 copies of hex addition `carry_chain_01` (642 MB), `dh20` and `dd20` 20 copies of hex and decimal division `small_00`, `mul18` 100,000 products of two 18-digit numbers, `msmall` 100,000 mixed short products. Each figure is the driver with the change against the driver without it; `B8` is the retired private-header driver on the same InfInt.

| Change | Measurement (cycles) | Decision |
|---|---|---|
| Base as a template argument instead of a runtime member | `dh20` 815 → 735M (B8 795), `dd20` 1085 → 1007M (B8 1469), `cc200` 930 → 912M (B8 915): the member cost 7–9% on short-token drivers in reloads, compare chains and spills across both paths | adopted |
| Inline long-token scan in `readToken` (the token's end found from the first 32-byte load or scanned on inline) | `cc200` tokenization stage 662.7 → 634.9M; whole driver at parity or better with GCC 16, GCC 14 and 64/32-byte function and loop alignment | adopted |
| Software prefetch 1 KiB ahead in the token scan | `cc200` 913 → 857M (B8 923), GCC 14 931 → 838M (B8 908), scalar 2094 → 1945M; cold cache (96 MB thrash before each run) hex addition `max_max_06` 5.645 → 5.419M (B8 5.655), `medium_01` 7.137 → 6.745M (B8 7.172), `max_01` −2.3%; LLC misses 3–6× fewer; distances 1024–4096 bytes equal within noise | adopted at 1024 |
| Masked Möller–Granlund correction in `writeDec(ulll)` (the branch form mispredicts about half the time) | `mul18` −4.7%, branch misses 115k → 18k; `msmall` −0.8% | adopted |
| Branch-light `writeDec(ulll)` without the `hi == 0` test | +6.5% up to 2.1× | rejected |
| Other token-scan forms (three vector variants, two refill variants), removing the status store | within ±1% | rejected |
| `parseInt` sets `Ok` on success (a stale `Overflow` or `Invalid` survived a later success of the inline path): the store at entry, against the store on success and one merged `Ok`/`Overflow` store | readInt instructions 0.989 (on success 1.022, merged 1.022), cycles 0.963–0.996 over `int` and `lng`, `N` = 65536 and 0 (on success 0.979–1.034); a bare `parseInt` loop at `N` = 65536 1.02–1.04 also aligned (on success 1.00–1.01); judge drivers instructions 1.000, cycles within the A/A spread (0.960–1.024) | adopted: the store at entry |
| `dec19`, `hex16`, `hex32` with one shared `return ok;` after the `#ifdef` (closing-brace rule) | `.text` identical to the earlier form on all six drivers in AVX2 and scalar builds; a `bool` accumulator in `dec19` and an early `return false` in `hex16` changed scalar code (the latter +0.55% instructions, +1–2% cycles on scalar hex division `small_00`, `medium_00`, `r_nearly_zero_00`) | shared `ok`, word accumulator in `dec19` |

### Pipeline benchmark

[`19-fastio_benchmark.py`](<../../96-Local Testing/01-Core/19-fastio_benchmark.py>) with its `.cpp` and `.json` times complete pipelines on a warmed regular file (construct, parse all input, transform, format, flush), compares every output byte outside the timed region with independent expectations (`to_chars` for integers, `snprintf("%.9f")` for doubles), one warmup and five repetitions with the method order rotated. 2026-11-01, `-std=gnu++20 -O2 -DNDEBUG` (no `-march`: the scalar paths), seed 20260927; `fast-whole` is `FastInput<0>`. Medians in milliseconds:

| Values / workload | Fast 4096 | Fast 65536 | Fast whole | fscanf/fprintf |
|---|---:|---:|---:|---:|
| 32 small integers | 0.0026 | 0.0025 | 0.0051 | 0.0047 |
| 32 full-width integers, mixed whitespace | 0.0026 | 0.0025 | 0.0054 | 0.0063 |
| 32 doubles (`%.17g` in, `%.9f` out) | 0.0039 | 0.0037 | 0.0071 | 0.0130 |
| 20,000 small integers | 0.627 | 0.576 | 0.621 | 1.823 |
| 20,000 full-width integers | 0.715 | 0.670 | 0.698 | 2.865 |
| 20,000 doubles | 1.849 | 1.800 | 1.791 | 8.440 |
| 500,000 small integers | 17.78 | 15.99 | 16.65 | 51.66 |
| 500,000 full-width integers | 21.69 | 18.75 | 18.70 | 79.58 |
| 500,000 doubles | 53.32 | 50.96 | 49.66 | 240.64 |

Shared-host observations, not timing gates. The whole-stream reader costs its `fstat` and mapping on tiny inputs (about 3 µs) and is within 8% of the 64 KiB buffer from 20,000 values and within 5% at 500,000 (faster on doubles and full-width integers); integers run 3.2–4.2 times and doubles 4.7 times the speed of `fscanf`/`fprintf` at 500,000 values.

## Sources

- Completeness sweep 2026-11-01 (`@researcher`, row 19): [00-sources.md](00-sources.md#fetched-2026-11-01-p018-stage-4-fast-io); left-out forms with reasons in [00-notes.md](00-notes.md#fast-io). Earlier sweeps (2026-09-27, 2026-10-07) of the 06 header: [06 sources](../../06-Miscellaneous/Docs/00-sources.md), rows `03` and `17`.
- [OI Wiki, 输入输出优化](https://oi-wiki.org/contest/io/): buffered `fread`/`fwrite`, signed-minimum handling, interactive flushing (read 2026-09-27).
- cppreference [fread](https://en.cppreference.com/w/c/io/fread.html), [fwrite](https://en.cppreference.com/w/c/io/fwrite.html), [fflush](https://en.cppreference.com/w/c/io/fflush.html): short transfers, EOF against `ferror`, delayed failure (2026-09-27); [from_chars](https://en.cppreference.com/w/cpp/utility/from_chars) and [to_chars](https://en.cppreference.com/w/cpp/utility/to_chars): correct rounding and `printf` equivalence of the floating forms.
- Linux man pages [mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html) (`MAP_PRIVATE`, `MAP_POPULATE`, which prefaults without failing the call; `munmap` needs a page-aligned address; a zero length is `EINVAL`, `MAP_FAILED` on error), [madvise(2)](https://man7.org/linux/man-pages/man2/madvise.2.html) (`MADV_HUGEPAGE` on a page-aligned range, advice that leaves semantics unchanged), [fstat(2)](https://man7.org/linux/man-pages/man2/fstat.2.html) (-1 on error) and [fileno(3)](https://man7.org/linux/man-pages/man3/fileno.3.html) (POSIX; -1 with `EBADF` for a stream without a descriptor, which takes the `fread` path): the whole-stream mapping and the long-text temporary (read 2026-11-01).
- Möller and Granlund, "Improved division by invariant integers" (IEEE Trans. Computers, 2011), as summarised in the GMP manual's Division Algorithms page (fetch record in [00-sources.md](00-sources.md#fetched-2026-10-17)): the 2-by-1 step of `writeDec(ulll)`, shared with InfInt ([07-infint.md](07-infint.md)).

## Limits and handoffs

- `readDouble` reports total underflow as `Overflow`; `long double` I/O and locale-specific formats are not owned. Floating `from_chars`/`to_chars` are in libstdc++ from GCC 11 (portable `double` parsing from GCC 12), which bounds portability to older judges.
- `FastInput<0>` holds the whole stream: a mapping of a regular file, else a vector that may reach twice the stream while it grows. Pipes and terminals take the vector path; interactive protocols use `FastInput<1>` with explicit flushes ([50-interactive.md](../../06-Miscellaneous/Docs/50-interactive.md)).
- `parseInt` takes only views this reader returned (its window precondition); other strings go through the integer types' own parsing.
- The short-token kernels repeat InfInt's on purpose (no Core type includes another for I/O); a change to one is measured and mirrored in the other.
- ModInt reading and writing (`rd`/`wt` of the archived modint) are not adopted: reading an integer and constructing the residue is a wrapper of `readInt` and the ModInt constructor ([03-cpp.md](../../00-Guidelines/03-cpp.md), One API per operation; [05-modint.md](05-modint.md)).
- Not owned, with reasons in [00-notes.md](00-notes.md#fast-io): count-returning and separator forms, lenient skipping, `bool` reading, fixed-length string reads, declare-and-read macros, `close`, stream setup.
- `readText` takes every type with `read(string_view, base)` (InfInt, InfIntMini, Rational), `writeText` only types with `charsBound` and `toChars` (InfInt): `print` and `writeValue` of an InfIntMini or a Rational do not compile, so such values print through `toString`.
- When the room left in the block lies between `toChars`' early lower bound and the exact length (in base 10 one digit below 152,234 bits and at most two within the default block, one more per about 217K bits beyond; about 1% of the digits in base 3), InfInt's `toCharsRest` converts and then refuses, and `writeText` converts again after its drain: a long value at a block boundary can cost two conversions.
- No Barrett or Montgomery reduction is involved.

## History

- 2026-09-27: P013 first verification as `06-Miscellaneous/03-fastio.hpp`, full and stress suites and benchmark passed.
- 2026-10-07: P013 re-audit, `readLine`, `readDouble`, `writeDouble`, `read`, `print`, `writeValue`, `end` and `precision` added (1 finding fixed); full, stress, benchmark and integration passed; `@reviewer` (2026-10-08) found no defects.
- 2026-10-12: P013 prune run (header unchanged), full suite passed on g++ and g++-14.
- 2026-11-01: P018 stage 4: moved to `01-Core/19-fastio.hpp` as a Core full header (no forwarding header in 06; P066's planned `MmapInput`/`SyscallOutput` row absorbed as `FastInput<0>` and `FastOutput`) with AVX2 and SWAR token kernels, the base as a template argument, `readText`/`writeText` for InfInt and the other text types, read-error tokens reported as `Error`, `parseInt`'s status set on success, `HUGE_PAGE`; `00-bigio.hpp` retired (drivers at or below it on every judge problem in cycles); new tester and Python protocols (one-unit sanitizer smoke subset under integration); `@reviewer` (Opus 5.5 at xhigh) findings fixed or kept by the user's decision; quick, full and stress suites passed on GCC 16 and full on GCC 14, sanitized integration and the benchmark passed.
- 2026-11-02: P018 `/finish` (Opus 5.5 at xhigh): the kernel renames `dec8`, `dec8Chars` and `dec16Chars` (identical machine code), Limits for the room gap of InfInt's out-of-line conversion and for the text types `readText` reads but `writeText` cannot print, Sources records for the man pages and Möller–Granlund; quick, full and stress suites passed on GCC 16 and full on GCC 14, sanitized integration, the six drivers and the judge reproduction passed.
