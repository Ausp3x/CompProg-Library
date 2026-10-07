---
paths:
  - "0[1-7]-*/**/*.hpp"
  - "**/*.cpp"
  - "96-Local Testing/**/*.py"
---

# C++

## Toolchain and judges

- Language: GNU C++20, `-std=gnu++20`, compiled with `-O2`. GNU extensions are allowed: `bits/stdc++.h`, PBDS, `__int128`, `__builtin_*`. Clang and MSVC are not targets. C++23 features are not allowed.
- Compiler floor: GCC 14.2. Target judges: Codeforces (GCC 14.2 MSYS2, Windows x86-64, `-static -Wl,--stack=268435456`, no `NDEBUG`), AtCoder (GCC 15.2 Linux, `-march=native`), Library Checker (GCC 15.2 Linux). Local development uses whatever newer GCC is installed; passing locally does not prove the floor.
- Windows consequences: `long` is 32-bit, so never use `long`, `unsigned long` or `%ld`; use the aliases below. Do not rely on `/dev/urandom`, `getrandom`, `mmap`, `sys/resource.h` or POSIX-only headers in library code. `std::random_device` and `std::chrono::steady_clock` are available everywhere.
- ISA kernels are compile-time guarded on `__AVX2__`, `__FMA__`, `__BMI2__`; the scalar path is the default. On judges without `-march=native` the user enables kernels with `#pragma GCC target("avx2,bmi2,fma")` before includes; the header never adds global pragmas or floating-point flags. No runtime dispatch.
- Judges run with assertions enabled. An assertion must not change an operation's asymptotic cost and must stay out of hot inner loops; check preconditions once at the entry of an operation.

## Headers

- `#pragma once`; include direct dependencies with relative paths, including `../01-Core/01-template.hpp` when aliases from it are used; compile alone, with every other header, and across multiple translation units (inline functions and variables or templates; no non-inline definitions in headers).
- No circular dependencies. No public name collisions between a full and a mini type.
- Full modular integers live in `05-modint.hpp` (`modint_detail`, `ModInt`, `ModInt64`, `DynModInt`, `DynModInt64`, alias `mint`). The four minis live in `06-modintmini.hpp`, each an independent struct containing its own helpers.

## Types

| Alias | Underlying | Use |
|---|---|---|
| `int` | | indices, sizes, counters that fit 31 bits |
| `uint` | `uint32_t` | packed words, masks, intentional wraparound |
| `lng` | `int64_t` | signed 64-bit values |
| `ulng` | `uint64_t` | unsigned 64-bit words and modular arithmetic |
| `lll` / `ulll` | `__int128_t` / `__uint128_t` | wide intermediates |

- Prefer `int` for indices; use `lng` only when 31 bits do not suffice. Pick intermediate widths as deliberately as stored widths: no silent overflow outside explicitly modular unsigned arithmetic.
- Use `size_t` only where a standard interface requires it (`std::bitset`, `index_sequence`, allocators). A container's `size()` being unsigned does not require unsigned loop variables.
- `int8_t`, `uint8_t`, `int16_t`, `uint16_t` have no aliases; use them as spelled. Aliases are exact type identities.
- Changing a signedness or width is a semantic change: review preconditions, overflow, comparisons, sentinels and deduction, and update tests.

## API contracts

- Zero-based indices and half-open ranges `[l, r)`; document any exception in the comment above the operation.
- Every struct and free function has a documented contract: domain and width limits, exactness, result meaning, mutation, setup or reset requirements, aliasing and ownership, cache lifetime and what invalidates it. The full text lives in the header's evidence document under `## Contracts`; the code carries only the two lines allowed in Comments below. A `64` suffix describes storage, not support for every 64-bit modulus; state the real range.
- Preconditions: `assert`. Valid no-answer results: a non-colliding sentinel, a `bool` status plus out-parameter, or a small status enum. Use `std::optional` only when those are more cumbersome. Absence is never the same value as a legitimate empty or zero result.
- Exact and approximate variants have distinct names and contracts even when they share code. State tolerances and coordinate bounds.
- Randomized algorithms expose a reproducible seed and state whether failure is Monte Carlo (wrong answer with probability p) or Las Vegas (runtime only).
- No `private`. State and helpers stay inside their struct; friend operators are encouraged; stateless algorithms are free functions.
- Shared helpers for one family go in one `<family>_detail` namespace in the same header, indented four spaces, closed with `} // namespace <family>_detail`, never re-exported with `using namespace`. No nested detail namespaces, no anonymous namespaces in headers, no `internal` or `utils` namespaces.

## Style

- Four-space indentation; no line-length limit.
- Names: structs `CamelCase`; functions and methods `camelCase`; fields and locals short lowercase or `snake_case`; compile-time constants `ALL_CAPS`; `constexpr` functions stay `camelCase`. Protocol names, operators, aliases such as `mint` and `iint`, template parameters such as `T` and `MOD`, and constructor size parameters such as `N` and `M` are exceptions.
- Use the same name for the same concept everywhere: `n` size, `m` second size or edge count, `l`/`r` range bounds, `i`/`j`/`k` indices, `u`/`v` vertices, `w` weight, `x`/`y` coordinates, `res` result.
- Braces on every `if`, `else`, loop and similar statement; ternaries excepted. Single-line blocks have spaces inside: `if (ok) { work(); }`.
- Closing braces: a multi-line block ends on the line of its last statement, `return res;}`. This applies to control blocks, function bodies and lambda bodies, and nests: `}}`. `else` starts a new line after `;}`. Exceptions: struct, class, union and enum bodies end with `};` on its own line; namespaces end with `} // namespace name` on its own line.
- One logical step per line: a short method may be one line; tightly coupled statements may share a line (`u = find(u); v = find(v);`); unrelated setup, mutation and branching are split. Keep loop headers and short braced conditionals compact.
- Group methods: construction, access, mutation, queries, operators; one blank line between groups; keep tiny overloads together. Never reorder data members to satisfy layout.
- Attach `*` and `&` to the name: `T *p`, `T &x`, `T &operator+=(const T &o)`.
- Casts: functional `T(x)` when unambiguous; `static_cast` otherwise; never hide a narrowing with a cast. Comparing a signed index with an unsigned size is acceptable when the bound is justified.
- Named lambdas: `auto f = [&](args) -> T { ... };` with the return type only when deduction is ambiguous. Recursive lambdas take `auto &&f` first and call `f(f, ...)`.
- Prefer `std::midpoint(l, r)` where its rounding fits. Constant factors first: `2 * n`, `32 * k`.
- Qualify standard names with `std::` except this closed list: `int8_t uint8_t int16_t uint16_t int32_t uint32_t int64_t uint64_t size_t ptrdiff_t string string_view istream ostream cin cout cerr pair tuple array bitset vector deque priority_queue queue stack map multimap unordered_map unordered_multimap set multiset unordered_set unordered_multiset abs max min gcd lcm reverse sort swap lower_bound upper_bound binary_search unique fill iota accumulate next prev`. Everything else, including `move`, `forward`, `exchange`, `midpoint`, traits and concepts, is qualified.

## Comments

- A struct or free function carries at most two comment lines directly above it: the complexity line, and one line naming the domain and the no-answer representation, for example `// mod in [1, 2^32); inv asserts a unit; sqrt returns -1 when no root.`
- Inside a body, a comment is one line and appears only where the code's trick or invariant is not evident from the code itself. No restated contracts, no history, no references to other documents.
- A file may open with one comment line. No block comments.
- Everything else (full contracts, staleness and invalidation rules, sentinel policies, proofs, provenance) goes to the header's evidence document under `## Contracts`, one subsection per struct or free function, linked from the inventory row.
- The validator rejects a verified header with more than two consecutive comment-only lines or with comment-only lines above 8% of its non-blank lines; one comment line is always allowed, so a one-function header keeps its complexity line.

## Complexity comments

- One comment line directly above every struct and free function except template and debug code: `// T: O(...), M: O(...)` for functions and simple structs, or `// S: O(...), U: O(...), Q: O(...), M: O(...)` for structures with setup, update and query. `NA` marks an inapplicable component.
- Symbols: `n`, `m`, `k`, `q` sizes and counts; `V`, `E` vertices and edges; `w` word width; `b` bit length; `L` string length; `S` alphabet size; `out` output size. Define any other symbol in the same comment. Write multiplication explicitly (`n * m`) and functions with parentheses (`log(n)`, `sqrt(n)`).
- State the worst case. Label `amortized` or `expected` bounds. Give conditions instead of `O(a) or O(b)`.
- `M` is stored data for a struct and auxiliary space for a function, excluding caller-owned inputs. Add workspace, cache or output terms when they exceed the stored bound: `M: O(n) (workspace O(n^2))`.
- A method gets its own comment only when its bound differs from the struct comment. ISA kernels need no comment; the public dispatcher's bound covers them. For big integers distinguish operation counts from bit complexity.
