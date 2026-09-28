# Basic buffered I/O — MI01 / P013

`03-fastio.hpp` independently implements `FastInput<N>` and `FastOutput<N>` with
64 KiB buffers by default. Scope is byte input/output, decimal integers, tokens,
EOF and explicit flush/error handling. Floating-point conversion, line parsing,
memory mapping, descriptor backends and platform-specific optimizations are not
implemented here; advanced backends remain with MI05 / `17-fast_io_advanced.hpp`.

## Contract and use

Both objects borrow a nonnull, open, byte-oriented `FILE*` (default `stdin` or
`stdout`). Its access mode must support the operation. Keep it alive through the
object's destructor. Neither object closes it, changes global stream settings,
or ties input to output. Copies and moves are deleted because duplicating a
buffer would duplicate or lose stream state. Empty construction/destruction has
no I/O side effects. Use one reader/writer per stream at a time; do not mix other
readers/writers or seek while the object is active. After destruction the caller
may rewind/reposition and reuse the `FILE*`. A reader does not return unused
read-ahead bytes to the file; external continuation starts at the file's actual
buffered position, so reposition explicitly when needed.

`N` is a positive compile-time `int`. Stored memory is `N` bytes plus constant
metadata per object, in addition to the C library's `FILE` buffering. Very large
automatic objects can exceed the stack limit; allocate them appropriately.
Time is linear in bytes consumed/written, including malformed tokens; integer
conversion has constant storage for each fixed-width type. `readToken` stores
its token temporarily, then moves it to the destination, using O(token length)
additional storage.

| API | Semantics |
|---|---|
| `peek()`, `get()` | Next unsigned byte `0..255`, or `EOF`; only `get` consumes it. Embedded NUL and high bytes are data. |
| `space(c)`, `skipSpace()` | Whitespace is exactly ASCII space, tab, LF, VT, FF, CR. `skipSpace` reports whether a nonspace byte remains. |
| `readChar(char&)` | Skip whitespace and consume one byte; preserve the destination on failure. |
| `readToken(string&)` | Skip whitespace and read a nonempty token, leaving the following delimiter unread; preserve the destination on failure. |
| `readInt(T&)` | Parse a whole whitespace-delimited decimal token. All GNU integral types except `bool`, including signed/unsigned 128-bit integers, are supported over their entire representable ranges. Optional `+`, leading zeros and signed `-0` are accepted. Unsigned negative forms, including `-0`, are invalid. |
| `put(char)`, `write(string_view)` | Append arbitrary bytes, including NUL. The view must not overlap the writer's internal buffer. Return whether the bytes have been buffered/transferred without a detected error. |
| `writeInt(T, end='\0')` | Decimal integral formatting; optional nonzero trailing byte. Use `put('\0')` for an actual NUL terminator. |
| `drain()` | Transfer the writer's buffer into the C stream; the C library may still buffer it. |
| `flush()` | Drain and call `fflush`; use this to observe deferred errors and before reading an interactive reply. |

Read operations set public `status`: `Ok`, `Eof`, `Invalid`, `Overflow` or
`Error`. Integer failures preserve the destination and consume the entire bad
token. A malformed overflow-length token is `Invalid`; an actual read error
before a complete token boundary takes precedence over both. A complete token
ending at ordinary EOF succeeds; a malformed or out-of-range token ending at
EOF reports its corresponding failure, and the next read reports `Eof`.
Invalid/overflow tokens do not poison later reads. Raw bytes already available
from a partial `fread` remain readable; its error is reported when those bytes
are exhausted. File errors require caller recovery after destroying the reader.

Output `error` is sticky after an observed short/failed `fwrite` or failed
`fflush`; further writes and flushes fail without retry. Some bytes may already
have reached the stream, and the unwritten remainder is discarded. The
destructor best-effort flushes pending output, including C-stream bytes from an
earlier `drain()`, but cannot report failure. Call `flush()` explicitly when
success matters. Error recovery requires a fresh writer after the caller fixes
the stream. Newline alone does not flush.

Bulk `fread` can wait to fill its requested count on a live pipe. Default input
is intended for batch input; `FastInput<1>` avoids bulk-fill waiting for ordinary
interactive token protocols. Such tokens still need a delimiter before a reply
can be processed. Always flush prompts explicitly. Nonblocking I/O, arbitrary
OS interruptions and retry policies are not part of this FILE-based API.

```cpp
FastInput<> in;
FastOutput<> out;
lng x;
while (in.readInt(x)) { out.writeInt(x, '\n'); }
bool ok = in.status == FastIoStatus::Eof && out.flush();
```

## Correctness argument

The reader keeps the unread half-open byte interval `[pos,len)` inside its
buffer, refilling only when empty. `fread` uses byte-sized elements, so returned
bytes are complete even on a partial read. Conversion accumulates an unsigned
magnitude `v`; before adding digit `d`, compare `v` against `limit / 10` and
`d` against `limit % 10`. The update therefore cannot overflow and never
exceeds the target's allowed magnitude. Signed negative values admit one more
magnitude than positive values. Constructing `-(v-1)-1` for nonzero negatives
keeps every signed intermediate representable, including the minimum integer.
Once overflow or syntax failure occurs, the parser continues to the token
boundary without assigning the destination.

Output obtains a negative input's magnitude through unsigned subtraction,
which is defined even for the signed minimum. Repeated quotient/remainder by
ten yields the exact decimal digits, and reversing them restores their order.
The fixed temporary array accommodates the largest decimal representation and
sign for every offered type. Buffer transfers preserve order; a short transfer
cannot be represented as success, and explicit flush includes errors deferred
by the C stream. These invariants explain correctness beyond finite testing.

## Source inspection

Read on 2026-09-27; code was independently implemented, with no copied source:

- [OI Wiki, 输入输出优化](https://oi-wiki.org/contest/io/), also retrieved as
  [the source page](https://raw.githubusercontent.com/OI-wiki/OI-wiki/master/docs/contest/io.md):
  buffered `fread`/`fwrite`, integer minimum overflow, explicit interactive flush,
  floating-point conversion as a separate family, and limits of mmap. Read the
  article and embedded buffer sketch; the linked implementation files and
  floating-point research papers were not used.
- cppreference [fread](https://en.cppreference.com/w/c/io/fread.html),
  [fwrite](https://en.cppreference.com/w/c/io/fwrite.html), and
  [fflush](https://en.cppreference.com/w/c/io/fflush.html): byte counts, short
  transfers, EOF versus `ferror`, and delayed output failure. Their cited C
  standard sections are reference pointers, not a claim of independent ISO
  standard review.

## Feature-to-test map and results

`96-Local Testing/06-Miscellaneous/03-fastio_tester.py` is runnable from any
directory. Tests use the actual header and retain checks under `-DNDEBUG`.

| Feature | Independent coverage |
|---|---|
| Decimal input/output and all boundaries | `std::to_chars` expected bytes; exhaustive signed/unsigned 8-bit domains in quick, 16-bit domains in full; deterministic random 32/64/128-bit values; full min/max and zero. Explicit `char`, wide/UTF character integer types, and `long long` type identities are instantiated. |
| Independent 128-bit parsing/formatting | Python arbitrary-precision decimal fixtures: 103 pairs quick, 3003 full, 30003 stress, signed minimum and unsigned maximum included. |
| Signs/syntax/overflow and recovery | Bare signs, repeated signs, punctuation, hex-like tokens, embedded NUL, byte 255, boundary ±1, 1000-digit tokens, malformed overflowing tokens, unsigned `-0`; both delimiter-terminated and EOF-terminated failures. |
| EOF/whitespace/buffer boundaries | Sizes 1, 7, 65536, empty and whitespace-only input, all six ASCII spaces, exact EOF after final digits, repeated EOF, all 256 raw bytes, NUL tokens, delimiter preservation. |
| Flush/lifetime/error behavior | `tmpfile` ownership reuse; underlying-descriptor observation of destructor flush after `drain`; GNU `fopencookie` short/failed reads and writes, error precedence and unchanged destinations, delayed `fflush` failure, sticky failure with no retries, empty lifetime without I/O; pipe tokens before writer closes using N=1. |
| Large byte strings | 10 KB quick, 1 MiB full, 8 MiB stress, compared byte-for-byte. |
| Preconditions | Checked-build null-input and null-output assertion probes; compile-time positive-buffer constraint and deleted copy/move operations. |

Validation on GCC 16.2.1 / GNU++20, seed `20260927`: full optimized `-O2
-DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and ASan/UBSan configurations pass.
Sanitizers were rerun outside the sandbox because its ptrace environment causes
LeakSanitizer to fail before it can report results. Quick mode also passed
optimized/checked. Stress optimized with seed `20260928` passes. The package
integration owner records standalone-header, aggregate and multiple-translation
unit checks separately.

```sh
python3 '96-Local Testing/06-Miscellaneous/03-fastio_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/06-Miscellaneous/03-fastio_tester.py' --mode stress --seed 20260928 --configuration optimized
python3 '96-Local Testing/06-Miscellaneous/90-fastio_benchmark.py'
```

## Performance observations

The reproducible `90-fastio_benchmark.{py,cpp,json}` compares a complete pipeline:
read signed 64-bit integers from a warmed regular file, transform bits, format
unsigned decimal lines, and flush the output. All output bytes are compared with
independent `std::to_chars` expectations outside timing. It measures buffers of
4096 and 65536 bytes against `fscanf`/`fprintf`, small/common/large input counts,
small integer tokens and full-width random values with mixed whitespace. Setup
file creation and fixture generation are excluded; each implementation includes
its own buffer construction and full parse/format/flush work. There is one
warmup and five repetitions with rotating method order.

Recorded on Intel Core i9-11900H, GCC 16.2.1, `-std=gnu++20 -O2 -DNDEBUG`, seed
20260927. Representative medians in milliseconds:

| Values / distribution | Fast 4096 | Fast 65536 | fscanf/fprintf |
|---|---:|---:|---:|
| 32 small integers | 0.00286 | 0.00281 | 0.00497 |
| 32 full width, mixed whitespace | 0.00391 | 0.00391 | 0.00642 |
| 20,000 small integers | 1.026 | 0.969 | 2.266 |
| 20,000 full width, mixed whitespace | 1.949 | 1.895 | 3.941 |
| 500,000 small integers | 26.059 | 25.280 | 61.330 |
| 500,000 full width, mixed whitespace | 47.610 | 46.135 | 94.234 |

These shared-host observations support the compact scalar buffered design and
default buffer for tested batch workloads. They are not timing gates or claims
about all devices/compilers. `fscanf`/`fprintf` also provide substantially broader
locale/format capabilities. Cold disk, networking, interactive throughput,
malformed-input throughput and floating-point conversion were not benchmarked.
