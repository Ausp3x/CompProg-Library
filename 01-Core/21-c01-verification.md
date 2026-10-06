# C01 template and debug foundation

The later [Core namespace migration](../00-Guidelines/22-core-migration.md) only added the labelled Debug namespace closing comment and refreshed its dependent Workspace snapshot. Earlier hashes below describe their original verification stage; the post-migration review at the end records current source hashes and fresh checks.

C01 owns `01-template.hpp` and `02-debug.hpp`. The package P002 integration record and bitset batch are separate. This batch preserves the original `OLD` and `97-Legacy` references and verifies the contracts below; it does not advertise general C++ reflection.

## Contracts and feature map

| Feature | Contract | Verification |
|---|---|---|
| Template imports and width aliases | GNU C++20, GCC14+, Linux and Windows (MinGW) x86-64; standard/PBDS includes; `uint`, `lng`, `ulng`, `lll`, `ulll` have 32/64/64/128/128 bits. No global ISA/FP flags or executable I/O initialization. | `01-template_tester.cpp::testAliases`; standalone and multiple-TU builds |
| Constants and macros | `INF32`/`INF64` are finite repeated-`0x3f` sentinels; they do not prevent arithmetic overflow. `fi`, `se`, `pb` retain the existing spelling. | `testMacrosAndConstants`: exact values, safe doubling, expanded field/insertion behavior |
| `indexed_set<T>`, `indexed_map<K, V>` | GNU PBDS red-black ordered unique set, and the same tree with a mapped type (`operator[]`, iterators to `pair<const K, V>`), `std::less` ordering; `find_by_order(k)` returns end outside size; `order_of_key(x)` counts keys strictly below x. Pair keys represent duplicates explicitly. O(log(n)) insertion/erase/rank/select, O(n) storage. | `testIndexedSet`: empty/out-of-range, duplicates, pair keys, deletion, map overwrite, deterministic random histories against independent `std::set`/`std::map` iteration |
| `chmin`/`chmax` | One comparison; assignment only on strict improvement; return whether assigned. Same-type arguments; comparison and assignment costs belong to T. Floating NaN never improves either operand. constexpr when T permits. | `testChminChmax`: constexpr, boundaries, ties, self-alias, strings, counted custom operations, NaN |
| Debug scalar/string output | Under `LOCAL`, `Debug::to_string(x)` returns a string. bool and bit proxies (`IS_BIT_PROXY`: class types convertible to bool with `flip()`, i.e. `vector<bool>::reference`, `bitset<N>::reference`, `Bitset::Reference`) spell `true`/`false`; char uses single quotes; integers use `std::to_string` (int8/uint8 numerically), with exact decimal 128-bit conversion; floating point (`float`, `double`, `long double`; not `__float128`) uses `std::to_chars` shortest round-trip text (`1.25`, `1e-09`, `-0`, `inf`, `nan`); enums print their promoted underlying value, except that a user `operator<<` streams instead (for byte-wide unscoped enums only when its text differs from the raw byte, since built-in promotion would otherwise print a character). Strings use double quotes and preserve raw bytes without escaping. Character arrays/pointers require NUL termination; null char pointers and `nullptr` print `nullptr`. | `testScalarsAndStrings`, `testFloatingRoundTrip`, `testOptionalVariantHooksAndWidths`, `testViewsAndRandomNesting`: signed/unsigned 128-bit extrema, bounded exhaustive small integers, all 256 char bytes, embedded NUL strings, signed/unsigned byte values, bitsets, floating literals/extrema/denormals/long double, seeded random double/float `strtod`/`strtof` round trips with a 17-significant-digit bound, scoped/unscoped/byte-wide/streamable enums (`uint8_t` and `char` unscoped enums print numbers, a scoped `char` enum with `operator<<` streams its character), proxies from slices, views, nested slices, `bitset` references and a user bit-proxy class |
| Tuple and aggregate output | Pair/tuple/custom `tuple_size` + ADL `get<I>` use parentheses (`Debug::element<I>`). Ranges take precedence over tuple protocol, so `std::array` uses braces. Legacy automatic aggregates (`AGG_SIZE_GEQ`/`AGG_SIZE_EXACT` arity predicates) support flat direct non-array/non-reference/non-bit-field members, without bases or anonymous unions, through eight members; empty aggregates print `{}`. Larger ordinary aggregates get `<aggregate: provide debugString>`. Other aggregate layouts require a hook or stream operator. | `testTuplesRangesAndAggregates`: sizes 0–9, tuple nesting, arrays; `testOptionalVariantHooksAndWidths`: custom tuple protocol, complex aggregate handled by hook |
| Ranges and views | Finite ranges print iteration order in braces, recursively. Const iteration is preferred; a range supporting only nonconst iteration is copied if copy-constructible. Copying such a range has its own time/storage cost. Single-pass iterators can consume shared external input; predicates/hooks/stream operators retain their own side effects. Values must remain alive throughout traversal. | Standard sequence/ordered/unordered containers, C arrays, spans, packed bools, filter/iota views; seeded nested vectors versus an independent stream-built expected result |
| Container adaptors | `Debug::container(adaptor)` returns the backing container by const reference: `queue` prints front to back; `stack` prints backing-container order, bottom to top; `priority_queue` prints heap backing order, not sorted removal order. The protected inherited member is accessed by a base-member pointer, with no downcast or element copy. | `testContainerAdaptors`: empty/populated adaptors, comparator variants, content and unchanged-state checks; move-only queue elements |
| Optional and variant | Absent optional prints `nullopt`; present optional prints `optional(value)`. Variant prints `variant[index](value)`, including repeated alternative types and monostate. Valueless-by-exception prints `variant(valueless)` without visiting. | Absent/present/nested optional; each variant alternative, duplicate types, monostate, and a throwing move assignment that produces the valueless state |
| Custom hooks | Provide `debugString(const T&)` in T's namespace returning a string-convertible result. ADL finds it for every nested value and gives it priority over ranges, tuples, aggregates and streaming. Hook implementations can recursively call `Debug::to_string`. `operator<<` remains the fallback for other streamable types. | Hook/stream precedence, hook/range precedence, nested optional/range hooks, array/reference aggregate hook, move-only hook |
| Slice | Legacy `Debug::slice(range, l, r, ...)` uses **inclusive** `[l, r]`; repeated index pairs slice nested ranges. Preconditions: `l >= 0`, `r >= l - 1`; extra endpoints clip, and `r == l - 1` is empty. Wide endpoint arithmetic does not overflow int. Lvalues are borrowed; supported rvalue ranges are owned, including prvalue subranges. Borrowed sources obey their iterator invalidation rules. | 1D/2D/3D, mutation through borrowed source, const/temporary source, empty/clipped/INT_MAX endpoints, owned nested temporaries and prvalue subranges; assertion failures for negative start and reversed endpoints |
| Output and tracing | `debugO` prints values/newline to `cerr`; `debug` includes expression text/source line and retains ANSI highlighting. `trace` emits scoped entry/exit, with unique guards even on one line. `Tracer` is neither copyable nor movable, must be destroyed in nesting order, and owns its label. `dep` is nonnegative and below INT_MAX on entry; direct user mutation during a live tracer is outside the contract. Indentation clamps safely to 128 spaces. | Exact output, empty argument list, expression/short-circuit/if-else contexts, multiple trace calls on one source line, nesting and ownership traits, extreme indentation, tracer precondition failure |
| Disabled macros | Without `LOCAL`, `debug(...)` and `trace(x)` expand to `void(0)` and do not evaluate or type-check their arguments. Debug helpers are then absent. | `02-debug_no-local_tester.cpp`: side effects, nonexistent names/types, loops, if/else and expression contexts |

`debug` evaluates each enabled argument once, with the normal C++ function-argument evaluation-order rules; expressions that depend on a particular order must be evaluated beforehand. Debug traversal is single-threaded, follows finite acyclic structures, and performs no graph cycle detection. Wide/UTF string types, arbitrary reflection, pointer-pointee traversal and format customization beyond hooks/streaming are outside this convenience layer; callers use hooks where needed. The hooks cover complex aggregate layouts instead of promising reflection unavailable in C++20.

## Correctness and cost rationale

128-bit conversion obtains a negative signed value's magnitude by conversion to unsigned followed by unsigned negation, so the minimum signed value is representable without signed overflow. Each division emits one decimal digit; the fixed 40-byte buffer covers all 39 digits of unsigned 128-bit values. Optional dereference and variant visitation occur only after checking their engaged/valueless states.

A single append writer recursively adds delimiters and each child into one result string, avoiding reconstruction/copying of complete nested subresults. For standard containers, scalar leaves and ordinary tuple/aggregate traversal it takes O(v + b) time and O(b + d) memory, where v is the visited values, b output bytes and d nesting depth; string growth is amortized. Custom hook/stream/iterator cost and any copied mutable-only range are additional. Container adaptors are inspected by const reference in O(n) traversal time without changing their backing storage. Printing a priority queue's heap order avoids an unnecessary O(n * log(n)) pop/sort traversal.

`slice` composes standard drop/take/transform views and forwards prvalue child ranges so owning subviews retain their elements. For random-access sized ranges, flat view setup is O(1); other ranges charge the underlying drop/take traversal (potentially O(l) to locate a start). Nested traversal charges each selected subrange's cost. View ownership incurs the source's move cost and owns its existing storage; borrowing adds no element storage.

These helpers have no material numeric/vector kernel warranting an ISA-specific path or a measured dispatch threshold. Template selection and standard-library string/container primitives are the appropriate implementation; no hardware acceleration claim is made. No standalone timing benchmark is needed to establish the append algorithm's output-sensitive bound.

## Legacy accounting and sources

- `OLD/1-Core/01-template.hpp` and `OLD/1-Core/02-debug.hpp` were inspected as the immediate feature/API references. The active template preserves their aliases, helper names, PBDS alias, constants and macros. The active debug preserves `Debug::to_string`, `debugO`, `slice`, `dep`, `indent`, `Tracer`, the aggregate arity predicates (renamed `AGG_SIZE_GEQ`/`AGG_SIZE_EXACT` in the 2026-10-06 re-audit) and the `debug`/`trace` macros.
- `01-Core/97-Legacy/01-template.cpp` and `02-debug.cpp` were inspected with their local index and the relevant migration notes. Their older `all(x)`, `ral(x)` and narrowing `sze(x)` macros had already been superseded by explicit begin/end/rbegin/rend/size expressions; they are not reintroduced. The older signed-width aliases are replaced by the active exact-width aliases. The older aggregate heuristic and invalid adaptor downcast are superseded by the explicit aggregate domain and legal pointer-to-member access. Original bytes remain in `OLD` and the local legacy excerpts.
- Deliberate debug output changes: null character pointers now print `nullptr` rather than an empty string; oversized aggregates show an explicit unsupported marker instead of misleading `{}`; signed/unsigned char values now print their numeric value (ordinary char retains quoted-byte output). Optional/variant/custom hooks are additions. Inclusive slice semantics and raw-byte string formatting are preserved.
- [cppreference, `std::visit`](https://en.cppreference.com/w/cpp/utility/variant/visit2.html), revision 187035, inspected 2026-09-27: visitation throws `bad_variant_access` for valueless input; one-variant dispatch has constant complexity independent of alternative count. This is the C++17/20 free function, not the similarly named C++26 member page. No external source code was copied.
- Installed GCC 16.2.1 libstdc++ headers were independently inspected on 2026-09-27: `/usr/include/c++/16/variant` lines 1952–1987 checks the valueless state before visitation; `/usr/include/c++/16/optional` lines 1250–1304 checks engagement before dereference and distinguishes throwing `value()`. `/usr/include/c++/16/bits/stl_queue.h` lines 152–161 and 591–594, and `stl_stack.h` lines 154–156 expose the standard protected backing-container member. These implementation checks support the standard API contracts and legal inherited-member access, rather than granting correctness to the old debug implementation.

## Original P002 verification commands and results

From any working directory, pass an absolute path to the entries; they resolve their actual headers relative to the script. Artifacts live in temporary directories. Terminals show in-place progress and green PASS/red FAIL unless `NO_COLOR` is set; redirected output uses plain lines. `CXX` selects the compiler, and the entries accept `--mode quick|full|stress` and `--seed`, or the run-all runner's `CP_TEST_MODE`/`CP_TEST_SEED` environment variables.

```bash
python3 '96-Local Testing/01-Core/01-template_tester.py' --mode full
python3 '96-Local Testing/01-Core/02-debug_tester.py' --mode full
python3 '96-Local Testing/01-Core/01-template_tester.py' --mode stress --seed 42
python3 '96-Local Testing/01-Core/02-debug_tester.py' --mode stress --seed 42
```

The full runs (seed 335597) and stress runs (seed 42, 30,000 histories per configuration) passed with GCC 16.2.1 (20260810), GNU++20, on Linux x86-64. Required configurations are optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_ASSERTIONS`, and `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie`. Leak detection was also enabled. Debug additionally passed `_GLIBCXX_DEBUG` checked execution. All oracles are non-removable checks. Quick uses the optimized configuration and 500 random histories; full runs 5,000 histories per configuration; stress uses the same full configuration set and 30,000 histories with the requested seed. Fixed boundary and formatting tests run in all modes.

The debug compile companion checks template/debug standalone inclusion in LOCAL/non-LOCAL, macro syntax, and same/mixed-LOCAL multiple-TU linkage and execution with `-Wall -Wextra -Werror`. Full checked debug execution also requires SIGABRT for the three documented invalid-precondition cases. Aggregate integration and generated workspace synchronization are recorded by the P002 integration owner.

Quick mode also passed after the display-only runner update; the debug entry was invoked by absolute path from `/tmp` to verify working-directory independence.

An initial sandbox sanitizer run completed all debug checks but LeakSanitizer then rejected the sandbox's ptrace environment; the successful full sanitizer executions were rerun outside that sandbox with approval. An initial PBDS `_GLIBCXX_DEBUG` run was impractically slow, so the template checked configuration uses `_GLIBCXX_ASSERTIONS`; this changes instrumentation, not the feature oracle or workload. This record does not claim GCC14 itself was executed, universal reflection, concurrency safety or online judge acceptance.

## Integer and style maintenance — 2026-09-27

The revised `03-cpp.md` integer/style defaults and the completed-package maintenance workflow were reread before this scoped update. `01-template.hpp` already met the defaults and is byte-for-byte unchanged: alias definitions and the tester’s independent checks against `uint32_t`, `int64_t`, `uint64_t`, `__int128_t` and `__uint128_t` are preserved.

Debug now separates logical formatter/tracer groups and output, setup and state-update steps. Its declaration order, public names, formatting behavior and brace style remain unchanged. Existing C++ testers use short locals, explicit standard qualification where required, functional 128-bit casts, and `int` for random nested dimensions/indices proven to lie in `[0,7]`. Test workloads and random draw order are unchanged. No new tests or features were added.

`size_t` remains for `std::bitset`, `std::index_sequence` and custom tuple protocol matching, the bounded conversion passed to `string_view`, and comparison with PBDS’s rank result. `ptrdiff_t` retains its view/iterator-difference role. Unsigned 128-bit magnitude arithmetic remains unsigned. Unbraced macro syntax fixtures intentionally exercise dangling-else behavior and were preserved.

Checks at integer/style maintenance with GCC 16.2.1, seed 335597, 5,000 iterations per configuration:

```bash
python3 '96-Local Testing/01-Core/01-template_tester.py' --mode full
python3 '96-Local Testing/01-Core/02-debug_tester.py' --mode full
```

Both passed: template 6/6 steps and debug 16/16 steps. These include optimized NDEBUG, checked assertions, ASan/UBSan with leak checking enabled, LOCAL/non-LOCAL behavior, three expected invalid-precondition aborts, standalone compilation, macro syntax and same/mixed-LOCAL multiple-TU linkage. The original stress results above are historical; stress and performance comparisons were not rerun for this maintenance.

Source hashes at integer/style maintenance, before the namespace migration (SHA256):

| File | SHA256 |
|---|---|
| `01-Core/01-template.hpp` | `251352f9cc65171c4527dc0596b40b21507652f0b2d0017c50431f67561964a2` |
| `01-Core/02-debug.hpp` | `cc29f0f453f7fdb698f24a643ec5e769102a0c333e1f15b3b920487845aefa8f` |
| `96-Local Testing/01-Core/01-template_tester.cpp` | `3886387adb1a60323cc7f4ace67a9ff665ed94ae2291a9f59a417376084209fd` |
| `96-Local Testing/01-Core/02-debug_tester.cpp` | `05d6e74987c91455c6655cfefee88c5b643feaf5967a45e7f3d24ed2dc1212f2` |
| `96-Local Testing/01-Core/02-debug_no-local_tester.cpp` | `c3e33b0b47ef866fc8f8effdd3c0727d2a8f3667b784052bf886ea3573d16306` |
| `96-Local Testing/01-Core/02-debug_compile_tester.py` | `4a4b5b1a82f73032b73484caabec07b71d41bed3b60b3abe6c2a238c203e6f0c` |

### Workspace handoff to SUP05/P003

At integer/style maintenance, the read-only `python3 '97-Online Testing/03-workspace.py' --check` reported a stale `99-Workspace/template.cpp` (exit 1) because its copied Debug layout predated that update. P002 left its explicit refresh and verification to SUP05/P003.

Resolved by [P003's refresh and verification](<../97-Online Testing/04-workspace-verification.md>) and the subsequent Core namespace migration. The post-migration review below confirms that `--check` now passes. No further template/debug change or Workspace refresh is needed; future input changes remain subject to coordination with SUP05/P003.

## Post-migration namespace review — 2026-09-27

Reviewed C01 before C15 against the current C++ guide, completed-package maintenance workflow, inventory/batch rows and Core migration record. `01-template.hpp` and `02-debug.hpp` already satisfy the current integer, grouping and namespace rules and remain byte-for-byte unchanged. In `02-debug_tester.cpp`, the `Custom` and `std` namespaces now have labelled closing comments and a separating blank line; the tuple fixture explicitly qualifies `std::integral_constant` and `std::conditional_t`. Test cases, draw order, type identities and behavior are unchanged. Current includes and Python companion/compile-fixture paths required no edit.

Alias definitions and independent underlying-type assertions remain intact. `size_t` still matches standard bitset/index-sequence/tuple protocols and PBDS rank results; the bounded `string_view` conversion and `ptrdiff_t` view differences retain their interface roles. Unsigned magnitude arithmetic and disabled-macro syntax fixtures are preserved. Public names, brace style and member/initializer order are unchanged.

Fresh checks with GCC 16.2.1, GNU++20, Linux x86-64:

```bash
python3 '96-Local Testing/01-Core/01-template_tester.py' --mode quick --seed 20260927
python3 '96-Local Testing/01-Core/02-debug_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-consistency.py'
python3 '97-Online Testing/03-workspace.py' --check
```

Template quick passed 2/2 steps with 500 histories. Debug full passed 16/16 with 5,000 histories per configuration: optimized NDEBUG, checked assertions, three expected precondition aborts, ASan/UBSan with leak checking, LOCAL/non-LOCAL behavior, standalone inclusion, macro syntax and same/mixed-LOCAL multiple-TU checks. A final-source sandbox run passed the sanitizer feature oracle but failed at LeakSanitizer shutdown because ptrace was unavailable; the unchanged suite then passed outside the sandbox with approval and leak checking retained. The earlier full/stress records remain historical; template full, stress and performance comparisons were not rerun for this review.

Consistency validation passed with 428 targets, 335 batches and 226 packages, including current dependent paths. Workspace `--check` passed read-only, and the snapshot is unchanged from the start of this review. No P002-owned gap or SUP05/P003 staleness remains. Shared inventories/checklists, other implementations and legacy originals were not edited.

Current source hashes (SHA256):

| File | SHA256 |
|---|---|
| `01-Core/01-template.hpp` | `251352f9cc65171c4527dc0596b40b21507652f0b2d0017c50431f67561964a2` |
| `01-Core/02-debug.hpp` | `2efc4730a89f75b079e5c394b9701cafb5dc38f7528fcd6f24df0a3795e8e08a` |
| `96-Local Testing/01-Core/01-template_tester.cpp` | `3886387adb1a60323cc7f4ace67a9ff665ed94ae2291a9f59a417376084209fd` |
| `96-Local Testing/01-Core/02-debug_tester.cpp` | `d818226112bfc42480d44140dd2ff2fde07ca2bbc142ddf72623166c6a3a6162` |
| `96-Local Testing/01-Core/02-debug_no-local_tester.cpp` | `c3e33b0b47ef866fc8f8effdd3c0727d2a8f3667b784052bf886ea3573d16306` |
| `96-Local Testing/01-Core/02-debug_compile_tester.py` | `4a4b5b1a82f73032b73484caabec07b71d41bed3b60b3abe6c2a238c203e6f0c` |
| `99-Workspace/template.cpp` | `11f9be9e307b483e7b1ee2bb2d47ddcf2a3a78356e84f7aa9726698128a29104` |

## Re-audit — 2026-10-06

Package P002 re-audit under the current `03-cpp.md`/`05-testing.md` rules, treating the previous verification as existing-unverified. Every operation in the inventory rows was compared with the code and the testers first; the full suites were rerun before any edit (template 6/6, debug 16/16, both passing on GCC 16.2.1) and again after.

### Confirmed findings and their disposition

| # | Finding ([P002.md](../00-Guidelines/23-reaudit-findings/00-index.md) at review time) | Disposition |
|---|---|---|
| 1 | Packed-bool proxies printed `1`/`0` | Fixed: `IS_BIT_PROXY` (class, bool-convertible, has `flip()`) joins the bool branch; covers `vector<bool>::reference`, `bitset<N>::reference` and `Bitset::Reference`. Tests: slice/view/nested slice of a non-const `vector<bool>`, `bitset` references, a user proxy class, and `Debug::to_string(Bitset::Reference)` in the bitset suite. |
| 2 | Row named the function-local `Accessor` instead of the public helpers | Fixed: row lists `container (queue, stack, priority_queue)`, `element`, `AGG_SIZE_GEQ`, `AGG_SIZE_EXACT` and `IS_BIT_PROXY`. |
| 3 | Header contract named only Linux | Fixed: `01-template.hpp` and this document state Linux and Windows (MinGW) x86-64. Correctness argument, not an executed test: the header uses PBDS, `__int128` and `<cstdint>` aliases only, no `long`, no POSIX calls; MinGW-w64 GCC 14 provides all of them. No Windows build was run (no cross compiler installed). |
| 4 | `aggSizGeq`/`aggSizExact` not ALL_CAPS | Fixed: renamed `AGG_SIZE_GEQ`/`AGG_SIZE_EXACT`; tester and the generated Workspace snapshot updated. |
| 5 | Struct bodies closing on a member line, `; }}` closers | Fixed: each `Accessor` and `Tracer` close with `};` alone; the variant block closes `';}}`; the tuple lambda closes `;}(...)`. |
| 9, 10 | Tester closing braces on their own lines | Fixed mechanically in `01-template_tester.cpp`, `02-debug_tester.cpp` and `02-debug_no-local_tester.cpp`; behavior unchanged. The new checker in `03-consistency.py` reports none. |

### Changes beyond the findings

- `indexed_map<K, V>` added to the template (KACTL OrderStatisticTree, hitonanode `pbds_map`); tested against `std::map` with the seeded history oracle.
- Floating-point formatting changed from `std::to_string` (fixed six decimals, so `1e-9` printed `0.000000`) to `std::to_chars` shortest round-trip text. Oracle: `strtod`/`strtof` must reproduce the exact value and sign for seeded random bit patterns, with at most 17 significant digits unless the text is an exact integer; literal expectations cover `1.25`, `1e-09`, `0.1`, `-0`, `100`, `1e+16`, `inf`, `-inf`, `nan`, `DBL_MAX`, the denormal minimum and a long double. `to_chars` for floating types is available on every target (libstdc++ 11+, including MinGW).
- Enums print their promoted underlying value instead of failing the static assertion (scoped) or printing a raw character (byte-wide unscoped, through built-in promotion); a user `operator<<` keeps precedence. Review found the first version let `enum E : uint8_t { V = 65 }` print `A`; the stream text is now compared with the raw byte for unscoped enums.
- The foundation runner compiles with `-Werror`, so new warnings fail the suite, and its precondition-abort cases require an assertion message on stderr.
- The inventory row names `IS_BIT_PROXY` and the formatting of int8/uint8, floating point and enums explicitly.

Catalog research for this re-audit is recorded in [81-sources.md](81-sources.md) (section "Fetched 2026-10-06 — P002 re-audit sweep"); exclusions with reasons are in [80-notes.md](80-notes.md) ("Template and debug").

### Commands and results

```bash
python3 '96-Local Testing/01-Core/01-template_tester.py' --mode full
python3 '96-Local Testing/01-Core/02-debug_tester.py' --mode full
python3 '96-Local Testing/01-Core/01-template_tester.py' --mode stress --seed 1
python3 '96-Local Testing/01-Core/02-debug_tester.py' --mode stress --seed 1
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/01-template.hpp 01-Core/02-debug.hpp '96-Local Testing/01-Core/01-template_tester.cpp' '96-Local Testing/01-Core/02-debug_tester.cpp' '96-Local Testing/01-Core/02-debug_no-local_tester.cpp'
python3 '97-Online Testing/03-workspace.py' && python3 '97-Online Testing/03-workspace.py' --check
```

All passed on GCC 16.2.1 (20260810), GNU++20, Linux x86-64, with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full (seed 335597, 5,000 histories per configuration): template 6/6 steps, debug 16/16 steps (optimized NDEBUG, checked `_GLIBCXX_ASSERTIONS`, ASan/UBSan with leak detection, three expected precondition aborts, LOCAL and non-LOCAL, standalone inclusion, macro syntax, same/mixed-LOCAL multiple translation units). Stress (seed 1, 30,000 histories per configuration): template and debug all configurations. The same full commands passed before the edits on the previous sources (template 6/6, debug 16/16). The closing-brace check reports no violations for the two headers and three testers. The Workspace snapshot was regenerated from the new debug header and `--check` passes. `02-integration.py --sanitizers` is recorded in [22-bitset.md](22-bitset.md#re-audit--2026-10-06). GCC 14 and a Windows/MinGW build were not executed; the floor and the Windows contract rest on the correctness arguments above.

Source hashes after the re-audit (SHA256):

| File | SHA256 |
|---|---|
| `01-Core/01-template.hpp` | `23e6db3f682fd9f27657ace73f157a8631790d06c8c72984d9fcc102de7341e6` |
| `01-Core/02-debug.hpp` | `dcd9207d671ce45e12e04d92ffc76852300f27ec962ad74ab4e5bd6a9a5b6d83` |
| `96-Local Testing/01-Core/01-template_tester.cpp` | `4fa8096a2989f42e10081aaef90a285161e5e6101731382d981b1d2d470d9d3d` |
| `96-Local Testing/01-Core/02-debug_tester.cpp` | `78dc1e099aeb8ccac8f74cb61e870d6faae0906c50b9bbd7484a1b5ccd99068d` |
| `96-Local Testing/01-Core/02-debug_no-local_tester.cpp` | `107af64fad30eafbe3c6bacd16f070a4be07454bd3299c288ef720c00615072d` |
| `96-Local Testing/01-Core/02-debug_compile_tester.py` | `4a4b5b1a82f73032b73484caabec07b71d41bed3b60b3abe6c2a238c203e6f0c` |
| `96-Local Testing/01-Core/_00_foundation_runner.py` | `1dd0641b66c8cd8cc14752738dceeb4c60bf44ecbfa0387d2a9dd5aa4fffd07d` |
| `99-Workspace/template.cpp` | `f6aa5003c6163e3fba441eb5508e21bc866cfe98faaccd37e1fc3ad59c8ea9df` |

Known limits: the bit-proxy rule treats any class convertible to bool with a `flip()` member as a proxy (a `debugString` hook overrides it); long double output is the 80-bit x87 shortest text on x86-64; `__float128` has no `to_chars` and is outside the domain; a user `operator<<` for a byte-wide unscoped enum that prints exactly the raw byte is indistinguishable from promotion and yields the number. Earlier sections keep their historical hashes and results.

### Closing-brace checker (owned work)

`96-Local Testing/03-consistency.py` gained `strip_cpp` (blanks comments, string/character literals with digit-separator and encoding-prefix awareness, raw strings and preprocessor lines, keeping columns) and `brace_violations` (tracks every `{` with its kind: namespace, type body, initializer or block; a multi-line block must close as `;}`/`}}` on its last statement's line and must not be followed by `else`; type bodies close as `};` alone; namespaces as `} // namespace name` alone; initializer braces after `=`, `(`, `,`, `[`, `{`, `return`, a range-for `:` or attached to a type name are exempt, `case`/`default` labels are blocks). It runs on every header whose inventory row and owning package are both `verified`, plus that header's C++ testers and benchmarks, and `--braces <files>` checks arbitrary files. Calibration: the eight P002 files report nothing after normalization (160 true positives before), and a probe file exercising own-line `}` after `f(){`, `case 1: {`, a struct head spanning lines, `u'a'`/`L'y'` literals, digit separators, range-for braces and a multi-line `= {` initializer reports exactly the four real violations. Independent review sampled the P004/P005 hits and found them all real. Recorded in `05-testing.md`.
