# 02-debug.hpp — Evidence

`02-debug.hpp` (C01, package P002 with `01-template.hpp` and `18-bitset.hpp`; see [01-template.md](01-template.md)) is the `LOCAL`-only formatting and tracing layer. It claims no general C++ reflection.

## Contracts

### Debug::to_string — Scalars and Strings

Under `LOCAL`, `Debug::to_string(x)` returns a string. bool and bit proxies (`IS_BIT_PROXY`: class types convertible to bool with `flip()`, i.e. `vector<bool>::reference`, `bitset<N>::reference`, `Bitset::Reference`) spell `true`/`false`; char uses single quotes; integers use `std::to_string` (int8/uint8 numerically), with exact decimal 128-bit conversion; floating point (`float`, `double`, `long double`; not `__float128`) uses `std::to_chars` shortest round-trip text (`1.25`, `1e-09`, `-0`, `inf`, `nan`); enums print their promoted underlying value, except that a user `operator<<` streams instead (for byte-wide unscoped enums only when its text differs from the raw byte, since built-in promotion would otherwise print a character). Strings use double quotes and preserve raw bytes without escaping. Character arrays/pointers require NUL termination; null char pointers and `nullptr` print `nullptr`.

### Tuples and Aggregates

Pair/tuple/custom `tuple_size` + ADL `get<I>` use parentheses (`Debug::element<I>`). Ranges take precedence over tuple protocol, so `std::array` uses braces. Legacy automatic aggregates (`AGG_SIZE_GEQ`/`AGG_SIZE_EXACT` arity predicates) support flat direct non-array/non-reference/non-bit-field members, without bases or anonymous unions, through eight members; empty aggregates print `{}`. Larger ordinary aggregates get `<aggregate: provide debugString>`. Other aggregate layouts require a hook or stream operator.

### Ranges and Views

Finite ranges print in iteration order in braces, recursively. Const iteration is preferred; a range with only nonconst iteration is copied if it is copy-constructible, at that copy's own time and storage cost. Single-pass iterators can consume shared external input, and predicates, hooks and stream operators keep their own side effects. Values must stay alive throughout traversal.

### Debug::container

`Debug::container(adaptor)` returns the backing container by const reference: `queue` prints front to back; `stack` prints backing-container order, bottom to top; `priority_queue` prints heap backing order, not sorted removal order. The protected inherited member is accessed by a base-member pointer, with no downcast or element copy.

### Optional and Variant

Absent optional prints `nullopt`; present optional prints `optional(value)`. Variant prints `variant[index](value)`, including repeated alternative types and monostate. Valueless-by-exception prints `variant(valueless)` without visiting.

### debugString hooks

A `debugString(const T&)` in T's namespace returning a string-convertible result is found by ADL for every nested value and takes priority over ranges, tuples, aggregates and streaming. A hook may call `Debug::to_string` recursively. `operator<<` stays the fallback for other streamable types.

### Debug::slice

Legacy `Debug::slice(range, l, r, ...)` uses **inclusive** `[l, r]`; repeated index pairs slice nested ranges. Preconditions: `l >= 0`, `r >= l - 1`; extra endpoints clip, and `r == l - 1` is empty. Wide endpoint arithmetic does not overflow int. Lvalues are borrowed; supported rvalue ranges are owned, including prvalue subranges. Borrowed sources obey their iterator invalidation rules.

### debugO, debug, trace, Tracer

`debugO` prints values/newline to `cerr`; `debug` includes expression text/source line and retains ANSI highlighting. `trace` emits scoped entry/exit, with unique guards even on one line. `Tracer` is neither copyable nor movable, must be destroyed in nesting order, and owns its label. `dep` is nonnegative and below INT_MAX on entry; direct user mutation during a live tracer is outside the contract. Indentation clamps safely to 128 spaces.

`debug` evaluates each enabled argument once, with the normal C++ function-argument evaluation-order rules; expressions that depend on a particular order must be evaluated beforehand. Debug traversal is single-threaded, follows finite acyclic structures, and performs no graph cycle detection.

### Disabled Macros

Without `LOCAL`, `debug(...)` and `trace(x)` expand to `void(0)` and do not evaluate or type-check their arguments. Debug helpers are then absent.

### Correctness and Cost

128-bit conversion takes a negative value's magnitude by converting to unsigned and negating there, so the minimum signed value needs no signed overflow. Each division emits one decimal digit, and the fixed 40-byte buffer holds all 39 digits of an unsigned 128-bit value. Optional dereference and variant visitation happen only after the engaged and valueless states are checked.

One append writer adds delimiters and each child recursively into one result string, so no nested subresult is rebuilt or copied. For standard containers, scalar leaves and ordinary tuple or aggregate traversal it takes O(v + b) time and O(b + d) memory, where v is the number of visited values, b the output bytes and d the nesting depth; string growth is amortized. Custom hooks, stream operators and iterators, and any copied nonconst-only range, cost extra. Container adaptors are read by const reference in O(n) without changing their storage; printing a priority queue in heap order avoids an O(n * log(n)) pop-and-sort.

`slice` composes standard drop, take and transform views and forwards prvalue child ranges, so owning subviews keep their elements. Flat view setup is O(1) on random-access sized ranges; other ranges pay the drop/take traversal (up to O(l) to find the start). Nested traversal pays each selected subrange's cost. An owning view pays the source's move and owns its existing storage; borrowing adds no element storage.

The helpers have no numeric or vector kernel that would warrant an ISA path or a measured threshold, so template selection over standard string and container primitives is the implementation and no hardware acceleration is claimed. The output-sensitive bound of the append writer needs no timing benchmark.

## Feature-to-Test Map

| Operation | Test | Oracle |
|---|---|---|
| Scalar/string output (`write`, `writeRange`), `IS_BITSET`, `IS_BIT_PROXY`, `IS_UNSCOPED_ENUM`, floating and enum formatting | `02-debug_tester.cpp`: `testScalarsAndStrings`, `testFloatingRoundTrip`, `testOptionalVariantHooksAndWidths`, `testViewsAndRandomNesting` | Signed/unsigned 128-bit extrema, bounded exhaustive small integers, all 256 char bytes, embedded NUL strings, signed/unsigned byte values, bitsets, floating literals/extrema/denormals/long double, seeded random double/float `strtod`/`strtof` round trips with a 17-significant-digit bound, scoped/unscoped/byte-wide/streamable enums (`uint8_t` and `char` unscoped enums print numbers, a scoped `char` enum with `operator<<` streams its character), proxies from slices, views, nested slices, `bitset` references and a user bit-proxy class; `Debug::to_string(Bitset::Reference)` in the bitset suite |
| Tuples, `element`, aggregates (`Any`, `AGG_SIZE_GEQ`, `AGG_SIZE_EXACT`) | `testTuplesRangesAndAggregates`; `testOptionalVariantHooksAndWidths` | Sizes 0–9, tuple nesting, arrays; custom tuple protocol, complex aggregate handled by hook |
| Ranges and views | `testViewsAndRandomNesting` and range groups | Standard sequence/ordered/unordered containers, C arrays, spans, packed bools, filter/iota views; seeded nested vectors versus an independent stream-built expected result |
| `container` (queue, stack, priority_queue) | `testContainerAdaptors` | Empty/populated adaptors, comparator variants, content and unchanged-state checks; move-only queue elements |
| Optional and variant (`IS_OPTIONAL`, `IS_VARIANT`) | `testOptionalVariantHooksAndWidths` | Absent/present/nested optional; each variant alternative, duplicate types, monostate, and a throwing move assignment that produces the valueless state |
| `debugString` hooks | Hook groups in `02-debug_tester.cpp` | Hook/stream precedence, hook/range precedence, nested optional/range hooks, array/reference aggregate hook, move-only hook |
| `slice` | Slice groups in `02-debug_tester.cpp` | 1D/2D/3D, mutation through borrowed source, const/temporary source, empty/clipped/INT_MAX endpoints, owned nested temporaries and prvalue subranges; assertion failures for negative start and reversed endpoints |
| `debugO`, `debug`, `trace`, `Tracer`, `dep`, `indent` | Output groups in `02-debug_tester.cpp` | Exact output, empty argument list, expression/short-circuit/if-else contexts, multiple trace calls on one source line, nesting and ownership traits, extreme indentation, tracer precondition failure |
| Disabled macros | `02-debug_no-local_tester.cpp` | Side effects, nonexistent names/types, loops, if/else and expression contexts |
| Standalone inclusion, macro syntax, multiple TUs | `02-debug_compile_tester.py` | Template/debug standalone inclusion in LOCAL/non-LOCAL, macro syntax, same/mixed-LOCAL multiple-TU linkage and execution with `-Wall -Wextra -Werror` |

Full mode also requires SIGABRT in the checked build for the three documented precondition violations. Quick runs the optimized configuration with 500 random histories, full 5,000 histories per configuration, and stress the full configuration set with 30,000 histories at the requested seed. The fixed boundary and formatting tests run in every mode.

## Commands and Results

Harness: [`05-testing.md`](../../00-Guidelines/05-testing.md); flags `--mode` and `--seed` (or `CP_TEST_MODE`/`CP_TEST_SEED`). The precondition-abort cases require an assertion message on stderr.

Library-wide runs of 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, run beside the other suites, build cache emptied before each cold run), all PASS:

| Command | Compiler | This suite | Memory peak |
|---|---|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` | GCC 16.2.1 | preflight quick peak 603 MB; 6 builds, 65.4 s compiling, 0.5 s running, longest run 0.3 s | 1385 MB |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | GCC 14.4.1 | preflight quick peak 613 MB; 6 builds, 100.8 s compiling, 0.9 s running, longest run 0.4 s | 1289 MB |
| `python3 '96-Local Testing/01-run.py' --mode quick` | GCC 16.2.1 | two optimized-NDEBUG builds, 21.4 s and 9.6 s compile, 0.0 s run each | 613 MB |
| `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | GCC 16.2.1 | preflight quick peak 319 MB; 6 builds, 32.0 s compiling, 0.8 s running, longest run 0.5 s | 1314 MB |

Restyle checks of 2026-10-08 (slice comment reworded, behavior unchanged), the latest run of each:

```bash
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/01-template.hpp 01-Core/02-debug.hpp 01-Core/18-bitset.hpp
python3 '96-Local Testing/01-Core/02-debug_tester.py' --mode full --seed 20261008
CXX=g++-14 python3 '96-Local Testing/01-Core/02-debug_tester.py' --mode full --seed 20261008
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '97-Online Testing/03-workspace.py' && python3 '97-Online Testing/03-workspace.py' --check
```

All passed on GCC 16.2.1 and GCC 14.4.1 (20260915), GNU++20, Linux x86-64, with `-Wall -Wextra -Wshadow -Wconversion -Werror`: full 16/16 steps on each compiler (optimized NDEBUG, checked `_GLIBCXX_ASSERTIONS`, ASan/UBSan with leak detection, three expected precondition aborts, LOCAL and non-LOCAL, standalone inclusion, macro syntax, same- and mixed-LOCAL multiple translation units), no brace or comment-cap violations, integration over 102 headers, consistency without errors, Workspace regenerated unchanged.

## Sources

- `OLD/1-Core/02-debug.hpp`: the feature and API reference. The active debug keeps `Debug::to_string`, `debugO`, `slice`, `dep`, `indent`, `Tracer`, the aggregate arity predicates (renamed `AGG_SIZE_GEQ`/`AGG_SIZE_EXACT`) and the `debug`/`trace` macros.
- `OLD/[1] algorithms.cpp` lines 31–228: its aggregate heuristic and invalid adaptor downcast are replaced by the explicit aggregate domain and legal pointer-to-member access.
- Deliberate output changes from the legacy debug: null character pointers print `nullptr`, not an empty string; oversized aggregates print an explicit unsupported marker, not a misleading `{}`; signed and unsigned char print their numeric value (plain char keeps quoted-byte output); floating point prints shortest round-trip text, not `std::to_string`'s six fixed decimals; enums print their underlying value. Optional, variant and custom hooks are additions. Inclusive slices and raw-byte strings are kept.
- [cppreference, `std::visit`](https://en.cppreference.com/w/cpp/utility/variant/visit2.html), revision 187035, inspected 2026-09-27: visitation throws `bad_variant_access` on valueless input, and one-variant dispatch takes constant time whatever the alternative count. This is the C++17/20 free function, not the C++26 member page of the same name. No external code was copied.
- Installed GCC 16.2.1 libstdc++ headers, inspected 2026-09-27: `/usr/include/c++/16/variant` lines 1952–1987 check the valueless state before visitation; `/usr/include/c++/16/optional` lines 1250–1304 check engagement before dereference and separate the throwing `value()`; `/usr/include/c++/16/bits/stl_queue.h` lines 152–161 and 591–594 and `stl_stack.h` lines 154–156 expose the standard protected backing-container member. They support the standard API contracts and the legal inherited-member access, not the old implementation.
- 2026-10-06 catalog sweep: [00-sources.md](00-sources.md).

## Limits and Handoffs

- Wide and UTF string types, arbitrary reflection, pointee traversal and format customization beyond hooks and streaming are outside this layer; callers use hooks where needed.
- The bit-proxy rule treats any class convertible to bool with a `flip()` member as a proxy; a `debugString` hook overrides it.
- `long double` prints the shortest 80-bit x87 text on x86-64; `__float128` has no `to_chars` and is outside the domain.
- A user `operator<<` for a byte-wide unscoped enum that prints exactly the raw byte cannot be told apart from promotion, so the number is printed.
- Items not adopted from the catalog sweep are listed in [00-notes.md](00-notes.md) ("Template and Debug").
- Verified before the mutation rule of 2026-11-03; no record yet.

## Aliases

Search aliases (P263, grab --find): 调试, 调试输出, 调试宏.

## History

- 2026-09-27: original P002 verification, full (seed 335597) and stress (seed 42) passed on GCC 16.2.1, including `_GLIBCXX_DEBUG`.
- 2026-09-27: integer/style maintenance, full suite passed 16/16; Workspace refresh handed to SUP05/P003.
- 2026-09-27: post-migration namespace review, full suite, consistency and workspace check passed.
- 2026-10-06: re-audit, 6 findings fixed (bit proxies, row names, ALL_CAPS predicates, closers, tester braces), shortest floating and enum formatting added, full and stress passed on g++.
- 2026-10-06 (run): full (seed 335597) and stress (seed 1) passed on g++, Workspace regenerated.
- 2026-10-08: restyle (slice comment reworded, behavior unchanged), full suite (seed 20261008, 16/16 steps) passed on g++ and g++-14, integration, consistency and workspace check passed.
- 2026-10-30: SUP18 arm64 portability (user decision 2026-10-30, fixed in P250): the `1.0L / 3` expectation follows the target's `long double` format (x87, binary128, binary64), x86-64 output unchanged; quick passed on x86-64 and on aarch64 under qemu-user, full passed on g++ and g++-14.
- 2026-11-07: P263 follow-up added Chinese search aliases (`grab --find`), Simplified plus Taiwan or Hong Kong terms where they differ; no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared ccache `Builder` in `_00_cpp_test_runner.py`, up to three at once within the memory cap, 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
