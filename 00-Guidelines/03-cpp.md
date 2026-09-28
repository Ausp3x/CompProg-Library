# C++ algorithms

Requires principles; read structure for file/API migrations and testing for verification.

## Baseline and dependencies

GNU C++20, GCC **14+**, `-std=gnu++20`, Linux x86-64 baseline. GNU headers/extensions (`bits/stdc++.h`, PBDS, `__int128`) are allowed; Clang/MSVC portability is not required. GCC still describes C++20 as almost fully implemented; this floor promises the library's tested feature subset, not every C++20 feature. Do not use modules or a newer standard implicitly.

Every header has `#pragma once` and includes its direct dependencies, including `01-template.hpp` when it uses its aliases/imports. It must compile alone, coexist with all other headers, and link safely across multiple translation units. Use inline definitions/variables or appropriate templates for header linkage. No circular dependencies. No public alias collisions between full/mini alternatives.

For Core full implementations, ISA choices are compile-time guarded with correct scalar fallback; do not use runtime CPU dispatch by default. FMA and any other specialization must preserve the stated numerical contract. No global unsafe floating-point flags as an incidental optimization. Domain/alignment/size assumptions of kernels must be enforced. Follow the profile and Barrett/Montgomery policies in principles.

## Integer types and aliases

Prefer `int` for algorithm indices, sizes and counters whose bounds fit, and `lng` when they require signed 64-bit range. This applies to maintained C++ algorithms and C++ examples/test support; Python integer conventions are unchanged. Select widths for intermediate arithmetic as well as stored values. Use unsigned types for packed words, masks and arithmetic that intentionally relies on unsigned semantics.

Use `size_t` only when an interface/template requires it or the supported size domain justifies it. Standard template matching such as `std::index_sequence`/`std::bitset`, and APIs intentionally supporting the full `size_t` range, are valid reasons. Retain `ptrdiff_t` or an iterator's difference type where pointer/iterator differences require that role. A container's `.size()` returning an unsigned type does not by itself require an unsigned loop variable; establish that its value fits before converting. Do not add casts merely to silence warnings, or substitute `ulng` just to disguise size semantics.

Prefer the shared aliases from `01-Core/01-template.hpp` over their underlying spellings:

| Preferred spelling | Underlying type |
|---|---|
| `uint` | `uint32_t` |
| `lng` | `int64_t` |
| `ulng` | `uint64_t` |
| `lll` | `__int128_t` |
| `ulll` | `__uint128_t` |

Use `int` for ordinary signed 32-bit values on the supported platform. Keep underlying types in alias definitions, independent type-identity checks, and interfaces where their particular type is required; retain `int8_t`, `uint8_t`, `int16_t` and `uint16_t` where needed because they have no project aliases. Aliases preserve exact type identity; do not assume every spelling of a same-width integer is the same C++ type. Standalone contest examples may define only the needed aliases locally using these exact underlying types; do not add a Core dependency solely for spelling convenience.

Changing signedness or width is a semantic edit: review negative-input preconditions, overflow, comparisons, sentinels, template deduction and narrowing, and update affected tests/contracts together. Preserve existing supported domains unless a domain change is explicitly part of the task. An exact alias substitution is a spelling change. Apply these defaults within the assigned scope; preserved `OLD`/`97-Legacy` references keep their original bytes.

## API contracts

Zero-based indexing and half-open `[l, r)` ranges by default; document a necessary exception. State domains, widths, exactness, result semantics, mutation, setup/reset requirements, and any ownership/aliasing assumptions. Suffix 64 describes storage intent, not automatically support for every unsigned 64-bit modulus; specify the actual supported range. Extend to every representable input when easy, otherwise prefer practical explicit bounds over many unnecessary branches. No silent overflow outside an explicitly modular unsigned computation.

Prefer `assert` for violated preconditions. Valid no-solution inputs need a noncolliding sentinel or simple status/result or status/out-parameter representation. Use `std::optional` only when those choices are infeasible or more cumbersome. Never conflate absence with a valid empty/zero result. Preconditions remain documented when assertions are disabled.

Exact and approximate APIs have distinct names/types/contracts; share implementation where useful. Record tolerances/error models and integer-coordinate bounds. Randomized algorithms are allowed: distinguish Monte Carlo answer risk from Las Vegas runtime randomness, describe assumptions/error bounds and expose reproducible seeds. State dynamic-modulus changes that invalidate existing values. Caches must have reset/invalidation and memory behavior documented.

Keep algorithm state/helpers inside its struct as reasonably possible, but **do not add private implementation sections**. This is a contest library. Friend operators/functions are encouraged where natural. Stateless algorithms can remain free functions.

## Namespaces and cohesive headers

Keep a helper used by one type inside that type. When several related public types need shared implementation, use one shallow, descriptively named `<family>_detail` namespace and put it in the same header as those types. Namespace size alone is not a reason for another file. Avoid nested detail layers, anonymous namespaces in headers, generic `internal`/`utils` namespaces, and public alias-only wrapper headers for a single cohesive family. Independently reusable algorithms such as Barrett and Montgomery retain their own headers and direct dependencies.

Indent namespace contents by four spaces. Group shared helpers before the public types, separate logical groups by one blank line, qualify shared names explicitly, and do not export a detail namespace with `using namespace`. Close a namespace on its own line with `} // namespace name`; this is an explicit exception to compressed control-block closing braces. Existing public namespace names remain stable during routine maintenance.

Full modular integers share `05-modint.hpp`: `modint_detail`, `ModInt`, `ModInt64`, `DynModInt`, and `DynModInt64`. The corresponding four mini types share `06-modintmini.hpp` for discovery, but each is one independently copyable struct. A mini may use standard headers and the documented template aliases; it must not depend on a sibling struct, full type, shared detail namespace, base class, or external reduction header. Put any required compact helpers/state inside that struct. Duplication needed for independent copying is intentional. Keep distinct full/mini names and preserve existing aliases such as `mint`.

## Compressed style

- Four spaces; no mandatory line limit. Preserve direct short code and intentional compressed braces.
- Group related functions/methods visually: use one blank line between logical groups such as construction, access, mutation, queries and operators; keep closely related tiny methods and overloads together. These are useful groups, not mandatory sections or a rigid ordering. Preserve data-member declaration order and declaration/lookup dependencies when arranging methods; formatting must not alter initialization or layout.
- Compress one logical step at a time. A short coherent method or tightly coupled statements may share a line; split lines that combine unrelated setup, mutation or branching. Keep ordinary loop control and concise braced conditionals compact. This does not impose one statement per line or replace the closing-brace style below.
- Always brace `if`, `else`, loops and similar control statements (ternary expressions excepted). Inline blocks have spaces inside: `if (ok) { work(); }`.
- Multiline closing braces attach to the final content where applicable:
  ```cpp
  if (ok) {
      work();}
  else {
      recover();}
  ```
  Struct/class and named lambda declarations keep their terminating `};` correctly. Do not change parsing to satisfy layout.
- Structs/classes: `CamelCase`. Methods/free functions/helpers: `camelCase`. Fields/locals: short lowercase or `snake_case`. Compile-time value constants: `ALL_CAPS`; runtime `const` values retain ordinary variable names. `constexpr` functions stay camelCase. Conventional protocol names/operators, short type aliases (`mint`, `iint`), mathematical template parameters (`T`, `MOD`) and constructor parameters (`N`, `M`) are exceptions.
- Use consistent vocabulary: prefer short mathematical locals such as `n`, `m`, `l`, `r`, `i` and `j` when their roles are clear, and recognizable domain names for public operations. Use the same term for the same concept across implementations; distinguish operations with different semantics. Routine style maintenance preserves existing public API/protocol names. Rename locals/helpers only when clearer and update affected uses; public API renaming requires an explicitly scoped migration.
- Attach `*`/`&` to names: `T *p`, `T &x`, `T &operator+=(...)`. Use concise functional numeric casts `T(x)` when unambiguous; use an appropriate explicit cast otherwise. Do not hide unsafe width conversions.
- Named nested lambdas use `auto f = [...] (...) -> T { ... };`; specify return type when deduction is ambiguous. Recursive lambdas take `auto &&f` first and call `f(f, ...)`.
- Prefer `std::midpoint(l, r)` when its rounding semantics fit; put constant factors before variable factors (`32 * n`). Avoid redundant temporary variables without obscuring invariants or repeating expensive work.
- Intentional signed/unsigned comparisons and safe conversions may retain warnings when bounds justify them. Signed/unsigned comparison is not synonymous with narrowing. Never dismiss all narrowing diagnostics wholesale.

## std:: qualification

The template may provide `using namespace std` and GNU PBDS imports. Nevertheless qualify standard names except this explicit convenience list:

`int8_t uint8_t int16_t uint16_t int32_t uint32_t int64_t uint64_t size_t ptrdiff_t string string_view istream ostream cin cout cerr pair tuple array bitset vector deque priority_queue queue stack map multimap unordered_map unordered_multimap set multiset unordered_set unordered_multiset abs max min gcd lcm reverse sort swap lower_bound upper_bound binary_search unique fill iota accumulate next prev`.

Built-in language types and library-defined aliases need no qualification. The list permits qualification omissions; it does not override the preferred integer spellings above. Explicit `std::` is always allowed to disambiguate. Other names, including `move`, `forward`, `exchange`, `midpoint`, traits/concepts and numeric utilities, remain qualified. Do not expand this list casually in individual headers.

## Complexity comments

Place immediately above algorithms, except template/debug. Use `// T: O(...), M: O(...)` or `// S: O(...), U: O(...), Q: O(...), M: O(...)`; use `NA` only for an inapplicable operation. Define needed symbols briefly: `n`, `m`, `k`, `V`, `E`, word width `w`, bit length `b`, output size, modulus, etc. Multiplication is explicit (`n * m`); functions use parentheses (`log(n)`, `ceil(n / w)`). State worst-case by default and label expected/amortized bounds. Explain conditions for alternate algorithms instead of ambiguous `O(x) or O(y)`.

For a struct, M is stored data; for a standalone function it is auxiliary space excluding caller-owned inputs. Identify returned storage if material. Add larger transient/shared costs explicitly, e.g. `M: O(n^2) (workspace: O(n^3), cache: O(n^3))`. Document output-sensitive bounds where needed. Per-method time comments are required only for bounds differing from the enclosing summary; add method memory notes only for a larger exceptional workspace/output/cache bound. Internal ISA kernels need not repeat public complexity comments, but public dispatch bounds must account for them. For big integers/stateful generic arithmetic, distinguish arithmetic-operation counts from bit complexity when unit-cost arithmetic would mislead.
