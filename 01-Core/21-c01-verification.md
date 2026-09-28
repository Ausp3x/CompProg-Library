# C01 template and debug foundation

The later [Core namespace migration](../00-Guidelines/22-core-migration.md) only added the labelled Debug namespace closing comment and refreshed its dependent Workspace snapshot. Earlier hashes below describe their original verification stage; the post-migration review at the end records current source hashes and fresh checks.

C01 owns `01-template.hpp` and `02-debug.hpp`. The package P002 integration record and bitset batch are separate. This batch preserves the original `OLD` and `97-Legacy` references and verifies the contracts below; it does not advertise general C++ reflection.

## Contracts and feature map

| Feature | Contract | Verification |
|---|---|---|
| Template imports and width aliases | GNU C++20, GCC14+, Linux x86-64; standard/PBDS includes; `uint`, `lng`, `ulng`, `lll`, `ulll` have 32/64/64/128/128 bits. No global ISA/FP flags or executable I/O initialization. | `01-template_tester.cpp::testAliases`; standalone and multiple-TU builds |
| Constants and macros | `INF32`/`INF64` are finite repeated-`0x3f` sentinels; they do not prevent arithmetic overflow. `fi`, `se`, `pb` retain the existing spelling. | `testMacrosAndConstants`: exact values, safe doubling, expanded field/insertion behavior |
| `indexed_set<T>` | GNU PBDS red-black ordered unique set, `std::less<T>` ordering; `find_by_order(k)` returns end outside size; `order_of_key(x)` counts keys strictly below x. Pair keys represent duplicates explicitly. O(log(n)) insertion/erase/rank/select, O(n) storage. | `testIndexedSet`: empty/out-of-range, duplicates, pair keys, deletion, deterministic random histories against independent `std::set` iteration |
| `chmin`/`chmax` | One comparison; assignment only on strict improvement; return whether assigned. Same-type arguments; comparison and assignment costs belong to T. Floating NaN never improves either operand. constexpr when T permits. | `testChminChmax`: constexpr, boundaries, ties, self-alias, strings, counted custom operations, NaN |
| Debug scalar/string output | Under `LOCAL`, `Debug::to_string(x)` returns a string. bool spells `true`/`false`; char uses single quotes; other arithmetic uses `std::to_string`, with exact decimal 128-bit conversion. Strings use double quotes and preserve raw bytes without escaping. Character arrays/pointers require NUL termination; null char pointers and `nullptr` print `nullptr`. | `testScalarsAndStrings`, `testOptionalVariantHooksAndWidths`: signed/unsigned 128-bit extrema, bounded exhaustive small integers, all 256 char bytes, embedded NUL strings, signed/unsigned byte values, bitsets, floats |
| Tuple and aggregate output | Pair/tuple/custom `tuple_size` + ADL `get<I>` use parentheses. Ranges take precedence over tuple protocol, so `std::array` uses braces. Legacy automatic aggregates support flat direct non-array/non-reference/non-bit-field members, without bases or anonymous unions, through eight members; empty aggregates print `{}`. Larger ordinary aggregates get `<aggregate: provide debugString>`. Other aggregate layouts require a hook or stream operator. | `testTuplesRangesAndAggregates`: sizes 0–9, tuple nesting, arrays; `testOptionalVariantHooksAndWidths`: custom tuple protocol, complex aggregate handled by hook |
| Ranges and views | Finite ranges print iteration order in braces, recursively. Const iteration is preferred; a range supporting only nonconst iteration is copied if copy-constructible. Copying such a range has its own time/storage cost. Single-pass iterators can consume shared external input; predicates/hooks/stream operators retain their own side effects. Values must remain alive throughout traversal. | Standard sequence/ordered/unordered containers, C arrays, spans, packed bools, filter/iota views; seeded nested vectors versus an independent stream-built expected result |
| Container adaptors | `queue` prints front to back; `stack` prints backing-container order, bottom to top; `priority_queue` prints heap backing order, not sorted removal order. The protected inherited member is accessed by a base-member pointer, with no downcast or element copy. | `testContainerAdaptors`: empty/populated adaptors, comparator variants, content and unchanged-state checks; move-only queue elements |
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

- `OLD/1-Core/01-template.hpp` and `OLD/1-Core/02-debug.hpp` were inspected as the immediate feature/API references. The active template preserves their aliases, helper names, PBDS alias, constants and macros. The active debug preserves `Debug::to_string`, `debugO`, `slice`, `dep`, `indent`, `Tracer`, aggregate arity predicates and the `debug`/`trace` macros.
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
