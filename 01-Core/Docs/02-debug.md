# 02-debug.hpp — evidence

`02-debug.hpp` (C01, package P002, together with `01-template.hpp`; see [01-template.md](01-template.md)) is the `LOCAL`-only formatting and tracing layer. It does not advertise general C++ reflection.

## Contracts

### Debug::to_string — scalars and strings

Under `LOCAL`, `Debug::to_string(x)` returns a string. bool and bit proxies (`IS_BIT_PROXY`: class types convertible to bool with `flip()`, i.e. `vector<bool>::reference`, `bitset<N>::reference`, `Bitset::Reference`) spell `true`/`false`; char uses single quotes; integers use `std::to_string` (int8/uint8 numerically), with exact decimal 128-bit conversion; floating point (`float`, `double`, `long double`; not `__float128`) uses `std::to_chars` shortest round-trip text (`1.25`, `1e-09`, `-0`, `inf`, `nan`); enums print their promoted underlying value, except that a user `operator<<` streams instead (for byte-wide unscoped enums only when its text differs from the raw byte, since built-in promotion would otherwise print a character). Strings use double quotes and preserve raw bytes without escaping. Character arrays/pointers require NUL termination; null char pointers and `nullptr` print `nullptr`.

### Tuples and aggregates

Pair/tuple/custom `tuple_size` + ADL `get<I>` use parentheses (`Debug::element<I>`). Ranges take precedence over tuple protocol, so `std::array` uses braces. Legacy automatic aggregates (`AGG_SIZE_GEQ`/`AGG_SIZE_EXACT` arity predicates) support flat direct non-array/non-reference/non-bit-field members, without bases or anonymous unions, through eight members; empty aggregates print `{}`. Larger ordinary aggregates get `<aggregate: provide debugString>`. Other aggregate layouts require a hook or stream operator.

### Ranges and views

Finite ranges print iteration order in braces, recursively. Const iteration is preferred; a range supporting only nonconst iteration is copied if copy-constructible. Copying such a range has its own time/storage cost. Single-pass iterators can consume shared external input; predicates/hooks/stream operators retain their own side effects. Values must remain alive throughout traversal.

### Debug::container

`Debug::container(adaptor)` returns the backing container by const reference: `queue` prints front to back; `stack` prints backing-container order, bottom to top; `priority_queue` prints heap backing order, not sorted removal order. The protected inherited member is accessed by a base-member pointer, with no downcast or element copy.

### Optional and variant

Absent optional prints `nullopt`; present optional prints `optional(value)`. Variant prints `variant[index](value)`, including repeated alternative types and monostate. Valueless-by-exception prints `variant(valueless)` without visiting.

### debugString hooks

Provide `debugString(const T&)` in T's namespace returning a string-convertible result. ADL finds it for every nested value and gives it priority over ranges, tuples, aggregates and streaming. Hook implementations can recursively call `Debug::to_string`. `operator<<` remains the fallback for other streamable types.

### Debug::slice

Legacy `Debug::slice(range, l, r, ...)` uses **inclusive** `[l, r]`; repeated index pairs slice nested ranges. Preconditions: `l >= 0`, `r >= l - 1`; extra endpoints clip, and `r == l - 1` is empty. Wide endpoint arithmetic does not overflow int. Lvalues are borrowed; supported rvalue ranges are owned, including prvalue subranges. Borrowed sources obey their iterator invalidation rules.

### debugO, debug, trace, Tracer

`debugO` prints values/newline to `cerr`; `debug` includes expression text/source line and retains ANSI highlighting. `trace` emits scoped entry/exit, with unique guards even on one line. `Tracer` is neither copyable nor movable, must be destroyed in nesting order, and owns its label. `dep` is nonnegative and below INT_MAX on entry; direct user mutation during a live tracer is outside the contract. Indentation clamps safely to 128 spaces.

`debug` evaluates each enabled argument once, with the normal C++ function-argument evaluation-order rules; expressions that depend on a particular order must be evaluated beforehand. Debug traversal is single-threaded, follows finite acyclic structures, and performs no graph cycle detection.

### Disabled macros

Without `LOCAL`, `debug(...)` and `trace(x)` expand to `void(0)` and do not evaluate or type-check their arguments. Debug helpers are then absent.

### Correctness and cost

128-bit conversion obtains a negative signed value's magnitude by conversion to unsigned followed by unsigned negation, so the minimum signed value is representable without signed overflow. Each division emits one decimal digit; the fixed 40-byte buffer covers all 39 digits of unsigned 128-bit values. Optional dereference and variant visitation occur only after checking their engaged/valueless states.

A single append writer recursively adds delimiters and each child into one result string, avoiding reconstruction/copying of complete nested subresults. For standard containers, scalar leaves and ordinary tuple/aggregate traversal it takes O(v + b) time and O(b + d) memory, where v is the visited values, b output bytes and d nesting depth; string growth is amortized. Custom hook/stream/iterator cost and any copied mutable-only range are additional. Container adaptors are inspected by const reference in O(n) traversal time without changing their backing storage. Printing a priority queue's heap order avoids an unnecessary O(n * log(n)) pop/sort traversal.

`slice` composes standard drop/take/transform views and forwards prvalue child ranges so owning subviews retain their elements. For random-access sized ranges, flat view setup is O(1); other ranges charge the underlying drop/take traversal (potentially O(l) to locate a start). Nested traversal charges each selected subrange's cost. View ownership incurs the source's move cost and owns its existing storage; borrowing adds no element storage.

These helpers have no material numeric/vector kernel warranting an ISA-specific path or a measured dispatch threshold. Template selection and standard-library string/container primitives are the appropriate implementation; no hardware acceleration claim is made. No standalone timing benchmark is needed to establish the append algorithm's output-sensitive bound.

## Feature-to-test map

| Operation | Test | Oracle |
|---|---|---|
| Scalar/string output, `IS_BIT_PROXY`, floating and enum formatting | `02-debug_tester.cpp`: `testScalarsAndStrings`, `testFloatingRoundTrip`, `testOptionalVariantHooksAndWidths`, `testViewsAndRandomNesting` | Signed/unsigned 128-bit extrema, bounded exhaustive small integers, all 256 char bytes, embedded NUL strings, signed/unsigned byte values, bitsets, floating literals/extrema/denormals/long double, seeded random double/float `strtod`/`strtof` round trips with a 17-significant-digit bound, scoped/unscoped/byte-wide/streamable enums (`uint8_t` and `char` unscoped enums print numbers, a scoped `char` enum with `operator<<` streams its character), proxies from slices, views, nested slices, `bitset` references and a user bit-proxy class; `Debug::to_string(Bitset::Reference)` in the bitset suite |
| Tuples, `element`, aggregates (`AGG_SIZE_GEQ`, `AGG_SIZE_EXACT`) | `testTuplesRangesAndAggregates`; `testOptionalVariantHooksAndWidths` | Sizes 0–9, tuple nesting, arrays; custom tuple protocol, complex aggregate handled by hook |
| Ranges and views | `testViewsAndRandomNesting` and range groups | Standard sequence/ordered/unordered containers, C arrays, spans, packed bools, filter/iota views; seeded nested vectors versus an independent stream-built expected result |
| `container` (queue, stack, priority_queue) | `testContainerAdaptors` | Empty/populated adaptors, comparator variants, content and unchanged-state checks; move-only queue elements |
| Optional and variant | `testOptionalVariantHooksAndWidths` | Absent/present/nested optional; each variant alternative, duplicate types, monostate, and a throwing move assignment that produces the valueless state |
| `debugString` hooks | Hook groups in `02-debug_tester.cpp` | Hook/stream precedence, hook/range precedence, nested optional/range hooks, array/reference aggregate hook, move-only hook |
| `slice` | Slice groups in `02-debug_tester.cpp` | 1D/2D/3D, mutation through borrowed source, const/temporary source, empty/clipped/INT_MAX endpoints, owned nested temporaries and prvalue subranges; assertion failures for negative start and reversed endpoints |
| `debugO`, `debug`, `trace`, `Tracer`, `dep`, `indent` | Output groups in `02-debug_tester.cpp` | Exact output, empty argument list, expression/short-circuit/if-else contexts, multiple trace calls on one source line, nesting and ownership traits, extreme indentation, tracer precondition failure |
| Disabled macros | `02-debug_no-local_tester.cpp` | Side effects, nonexistent names/types, loops, if/else and expression contexts |
| Standalone inclusion, macro syntax, multiple TUs | `02-debug_compile_tester.py` | Template/debug standalone inclusion in LOCAL/non-LOCAL, macro syntax, same/mixed-LOCAL multiple-TU linkage and execution with `-Wall -Wextra -Werror` |

Full checked debug execution also requires SIGABRT for the three documented invalid-precondition cases. Quick uses the optimized configuration and 500 random histories; full runs 5,000 histories per configuration; stress uses the same full configuration set and 30,000 histories with the requested seed. Fixed boundary and formatting tests run in all modes. All oracles are non-removable checks.

## Commands and results

The entries resolve their headers relative to the script and run from any working directory; `CXX` selects the compiler, and they accept `--mode quick|full|stress` and `--seed` (or `CP_TEST_MODE`/`CP_TEST_SEED`). The runner compiles with `-Werror`, and its precondition-abort cases require an assertion message on stderr.

Restyle, 2026-10-08 (slice comment reworded; behavior unchanged):

```bash
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/01-template.hpp 01-Core/02-debug.hpp 01-Core/18-bitset.hpp
python3 '96-Local Testing/01-Core/02-debug_tester.py' --mode full --seed 20261008
CXX=g++-14 python3 '96-Local Testing/01-Core/02-debug_tester.py' --mode full --seed 20261008
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '97-Online Testing/03-workspace.py' && python3 '97-Online Testing/03-workspace.py' --check
```

All passed on GCC 16.2.1 and GCC 14.4.1 (20260915), GNU++20, Linux x86-64, with `-Wall -Wextra -Wshadow -Wconversion -Werror`: full 16/16 steps on each compiler (optimized NDEBUG, checked `_GLIBCXX_ASSERTIONS`, ASan/UBSan with leak detection, three expected precondition aborts, LOCAL and non-LOCAL, standalone inclusion, macro syntax, same/mixed-LOCAL multiple translation units), no brace or comment-cap violations, integration over 102 headers, consistency without errors, Workspace regenerated unchanged.

## Sources

- `OLD/1-Core/02-debug.hpp`: immediate feature/API reference. The active debug preserves `Debug::to_string`, `debugO`, `slice`, `dep`, `indent`, `Tracer`, the aggregate arity predicates (renamed `AGG_SIZE_GEQ`/`AGG_SIZE_EXACT`) and the `debug`/`trace` macros.
- `01-Core/97-Legacy/02-debug.cpp`: the older aggregate heuristic and invalid adaptor downcast are superseded by the explicit aggregate domain and legal pointer-to-member access. Original bytes remain in `OLD`.
- Deliberate output changes against the legacy debug: null character pointers print `nullptr` rather than an empty string; oversized aggregates show an explicit unsupported marker instead of misleading `{}`; signed/unsigned char values print their numeric value (ordinary char retains quoted-byte output); floating point prints shortest round-trip text instead of `std::to_string` fixed six decimals; enums print their underlying value. Optional/variant/custom hooks are additions. Inclusive slice semantics and raw-byte string formatting are preserved.
- [cppreference, `std::visit`](https://en.cppreference.com/w/cpp/utility/variant/visit2.html), revision 187035, inspected 2026-09-27: visitation throws `bad_variant_access` for valueless input; one-variant dispatch has constant complexity independent of alternative count. This is the C++17/20 free function, not the similarly named C++26 member page. No external source code was copied.
- Installed GCC 16.2.1 libstdc++ headers, independently inspected on 2026-09-27: `/usr/include/c++/16/variant` lines 1952–1987 checks the valueless state before visitation; `/usr/include/c++/16/optional` lines 1250–1304 checks engagement before dereference and distinguishes throwing `value()`. `/usr/include/c++/16/bits/stl_queue.h` lines 152–161 and 591–594, and `stl_stack.h` lines 154–156 expose the standard protected backing-container member. These checks support the standard API contracts and legal inherited-member access, rather than granting correctness to the old debug implementation.
- 2026-10-06 catalog sweep: [00-sources.md](00-sources.md).

## Limits and handoffs

Wide/UTF string types, arbitrary reflection, pointer-pointee traversal and format customization beyond hooks/streaming are outside this convenience layer; callers use hooks where needed. The bit-proxy rule treats any class convertible to bool with a `flip()` member as a proxy (a `debugString` hook overrides it); long double output is the 80-bit x87 shortest text on x86-64; `__float128` has no `to_chars` and is outside the domain; a user `operator<<` for a byte-wide unscoped enum that prints exactly the raw byte is indistinguishable from promotion and yields the number. GCC 14.4.1 (`CXX=g++-14`, the floor check) also passed; exact GCC 14.2 and a Windows/MinGW build were not executed. Items not adopted from the catalog sweep are listed in [00-notes.md](00-notes.md) ("Template and debug"). Workspace regeneration stays coordinated with SUP05/P003.

## History

- 2026-09-27: original P002 verification, full (seed 335597) and stress (seed 42) passed on GCC 16.2.1, including `_GLIBCXX_DEBUG`.
- 2026-09-27: integer/style maintenance, full suite passed 16/16; Workspace refresh handed to SUP05/P003.
- 2026-09-27: post-migration namespace review, full suite, consistency and workspace check passed.
- 2026-10-06: re-audit, 6 findings fixed (bit proxies, row names, ALL_CAPS predicates, closers, tester braces), shortest floating and enum formatting added, full and stress passed on g++.
- 2026-10-06 (run): full (seed 335597) and stress (seed 1) passed on g++, Workspace regenerated.
