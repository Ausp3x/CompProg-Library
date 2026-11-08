---
paths:
  - "0[1-7]-*/**/*.hpp"
  - "**/*.cpp"
  - "96-Local Testing/**/*.py"
---

# C++

## Toolchain and judges

- Language: GNU C++20, `-std=gnu++20`, compiled with `-O2`. GNU extensions are allowed: `bits/stdc++.h`, PBDS, `__int128`, `__builtin_*`. Clang and MSVC are not targets. C++23 features are not allowed.
- Compiler floor: GCC 14, checked with the local `g++-14`. Target judges: Codeforces (GCC 14, Windows x86-64, `-static -Wl,--stack=268435456`, no `NDEBUG`), AtCoder (GCC 15.2 Linux, `-march=native`), Library Checker (GCC 15.2 Linux). Passing on a newer local GCC does not prove the floor.
- Codeforces portability: `long` is 32-bit there, so never use `long`, `unsigned long` or `%ld`; use the aliases below. Do not rely on `/dev/urandom`, `getrandom`, `mmap`, `sys/resource.h` or POSIX-only headers in library code; the exception is a Core full header, which may call `mmap`, `munmap`, `madvise`, `fstat` and `fileno` under `#ifdef __linux__` when a portable path without them gives identical results (InfInt's arena, the fast I/O reader and writer). `std::random_device` and `std::chrono::steady_clock` are available everywhere.
- ISA kernels are compile-time guarded on `__AVX2__`, `__FMA__`, `__BMI2__`; the scalar path is the default. On judges without `-march=native` the user enables kernels with `#pragma GCC target("avx2,bmi2,fma")` before includes; the header never adds global pragmas or floating-point flags. No runtime dispatch.
- Judges run with assertions enabled. An assertion must not change an operation's asymptotic cost and must stay out of hot inner loops; check preconditions once at the entry of an operation.
- A violated precondition must stay bounded when `NDEBUG` removes the assertion: no infinite loop and no unbounded allocation (for example `while (n > 1 && n % p == 0)` instead of relying on `assert(n >= 1)`). The result may be meaningless; the process must still terminate within the operation's normal bounds.

## Headers

- `#pragma once`; include direct dependencies with relative paths, including `../01-Core/01-template.hpp` when aliases from it are used; compile alone, with every other header, and across multiple translation units (inline functions and variables or templates; no non-inline definitions in headers).
- No circular dependencies. No public name collisions between a full and a mini type.
- No `#define` or `#undef` outside `01-template.hpp` and `02-debug.hpp`: extraction copies declarations, never macros (`05-testing.md`, Extraction; user decision 2026-11-06).
- A deduced template parameter that stands for a library struct its header's include closure declares must default to it (`template<class G = Graph>`, `template<typename P = point>`, `template<class L = LCA>`; a container of such structs too, `template<typename Polygon = vector<point>>`), so `grab` of the function brings the type, and the extraction probes instantiate the function where its other parameters have defaults too; calls still deduce. Callables, other sequences, scalars, problem-chosen monoids and Core types (Core type parameters) take none, and a header does not include a type's header only for a default (user decision 2026-11-06). grab infers a missing one from the members the template uses, and `05-extraction.py ops` fails on it in verified rows (`05-testing.md`, Extraction).
- Core type parameters: a contest header (`02`–`07`) takes each Core type or reducer an operation computes with as a template parameter, which dependents forward. Its default is the mini or dependency-free code in the family's detail namespace; a full type runs only when the caller passes it (`phi<Montgomery64>(n)`, `bellTable<ModInt<998244353>>(n)`). The short aliases `mint`, `iint` and `FPS` belong to the minis; full types go by their struct names. A full type without a mini that has no ISA code or reduction backend and includes only `01-template.hpp` and minis (today `Rational`) counts as a mini here. Generic code uses only operations both tiers provide (`02-structure.md`, Inventory contract), detects capabilities with `requires`, and names no full type or full-only concept. An operation that needs a capability its mini still lacks records `full-only until <mini operation>` in its evidence document while that operation is listed after `missing:` on the mini's `partial` row (`02-structure.md`, Inventory contract), or until the user adds a mini for a type that has none.

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
- Every struct and free function has a documented contract (a type's `friend` algorithms and `static` builders are part of the type's contract): domain and width limits, exactness, result meaning, mutation, setup or reset requirements, aliasing and ownership, cache lifetime and what invalidates it. A `64` suffix describes storage, not support for every 64-bit modulus; state the real range.
- Preconditions: `assert`. Valid no-answer results: a non-colliding sentinel, a `bool` status plus out-parameter, or a small status enum. Use `std::optional` only when those are more cumbersome. Absence is never the same value as a legitimate empty or zero result.
- Exact and approximate variants have distinct names and contracts even when they share code. State tolerances and coordinate bounds.
- Randomized algorithms expose a reproducible seed and state whether failure is Monte Carlo (wrong answer with probability p) or Las Vegas (runtime only).
- An operation over an associative fold takes a monoid `M` (`S`, `op`, `e`; an acted monoid adds `F`, `mapping`, `composition` new after old, `id`), never a separate identity and function. A structure stores it as `[[no_unique_address]] M m`, constructors and functions take a defaulted `M mon = {}` as their last argument (`LCAFold` keeps its optional `root` after it), and calls go through the object (`m.op`), so presets, stateful structs and lambdas through `Monoid`/`ActedMonoid` all work. Range products are `prod(l, r)` and `allProd()` (products along a walk or tree path keep `fold` and `pathFold`); carrying a value `x` through a range under an action of the monoid is `fold(l, r, x, act)` (left to right, `act(x, s)`) or `foldRight(l, r, x, act)` (right to left, `act(s, x)`). Presets, adapters and laws: row 00 of `02-Data Structures`.
- No `private`. State and helpers stay inside their struct.
- Type headers have no public free functions. A type header is every `01-Core` header except `01-template.hpp` and `02-debug.hpp` (full and mini), and every `02-Data Structures` header whose inventory row starts with a struct (structs and their operations, as opposed to rows of standalone functions such as `08-monotone_stack.hpp`). Every public operation of the type sits with the type:
  - An operation that takes at least one value of the type is a `friend` defined inside the struct, operators included (`exp(f, n)`, `gcd(a, b)`, `a * b`). It is found only through ADL, so it never collides with a `std::` or Mathematics function of the same name.
  - An operation that builds a value of the type from other data is a `static` member (`Poly<T>::interpolate(xs, ys)`, `Poly<T>::cyclotomic(n)`).
  - An operation of the family that does not depend on the type's template parameters is a `static` member of one non-template companion struct named `<Type>Companion`, declared directly after the type (`PolyCompanion::convolutionLng(a, b)`). Companion functions keep the family word in their names. A header has at most one companion; a non-template type puts these in the type itself.
  - Kernels, caches and helpers shared by these stay in `<family>_detail` and are not public API.
  - A mini (`*mini.hpp`) follows the same rules under its own names (`PolyMiniCompanion`, `polymini_detail`).
- Every other `02`–`07` header keeps free functions, including algorithm headers that define result or helper structs (suffix array, suffix automaton, dominator tree, compression and similar builders) and the geometry primitives and graph representations whose operations are free (`03-Geometry/01-point.hpp`, `02-line_segment.hpp`, `06-circle.hpp`, `04-Graphs/01-graph.hpp`); never wrap an algorithm in a struct only to hold it.
- Shared helpers for one family go in one `<family>_detail` namespace in the same header, indented four spaces, closed with `} // namespace <family>_detail`, never re-exported with `using namespace`. No nested detail namespaces, no anonymous namespaces in headers, no `internal` or `utils` namespaces.

## One API per operation

- An operation has one form per context. A context is where a reader looks for it: a type's members, a row's family, or a named family inside a row (unlabelled beside labelled constructions).
- An operation or header is removed only when another public operation in the same context already has exactly its purpose and results and the only differences are these forms: another function name, an index base or range convention, an argument or result shape (out-parameters, a pair with a sentinel, a flag choosing between operations, a projected field, a one-vertex overload of a multi-source function), a legacy default argument (`modPow`'s `INF64` modulus), a narrowing, a legacy input format a builder converts, a second include path. A second implementation of an algorithm, with the same domain and results as an existing operation, goes too. Neither is added; the owning package removes existing ones (`/prune`), and its evidence document maps each removed name to its replacement.
- Any other difference, however small, makes a separate operation that stays: a different algorithm, a slower one when `01-principles.md` (Priority) admits it (Edmonds–Karp beside Dinic, Kuhn beside Hopcroft–Karp); another unit, scale or derived formula (`Timer::elapsedMs` beside `elapsedSec`, `ceilDiv` beside `floorDiv`); a named special case or batch form (`Fenwick::get`, `lastTrue`, `TwoSat::setValue`, `phiTable`, `CoordinateCompression::ranks`); a larger or different domain, including inputs the other rejects (`maxZeroSubmatrix`, `LiChaoTree`); reuse of a caller's precomputation (`fctGcd`, `convexHullSorted`). An operation's only form stays even when closed-range or sentinel-based (`Debug::slice`, `solveModEq`).
- Wrappers and compositions stay too: a wrapper offering an operation in another context (Mathematics `Permutation` beside Miscellaneous row 07, `Ball::rationalInInterval`, `unlabelledSequence` beside `labelledSequence`), and a composition of public operations that any source names as a task, one-problem tricks included (`bernoulliPolynomial`, `staticRangeInversionsFenwick`). Each calls the operations it wraps or composes; a copy of their code needs a case-by-case reason, such as cutting a heavy dependency, recorded in the folder's `Docs/00-notes.md`. Its name follows `02-structure.md` (Operation names).
- Kept by decision although they differ only in form: type aliases and the template macros, `tryX`/`X` pairs, and intended duplication (minis, `08-Python`, the Mathematics duplicates `02-structure.md` allows). Public data members and helpers public only because the library has no `private` are not operations.
- A pinned unchanged extraction (`03-consistency.py` checks its `OLD` hash) keeps its body until its package rewrites it; only its inventory row drops the duplicates now.
- Decisions and the confirmed list: `00-Guidelines/23-Sweep Ledgers/duplicates.md`.

## Style

- Four-space indentation; no line-length limit.
- Names: structs `CamelCase`; functions and methods `camelCase`; fields and locals short lowercase or `snake_case`; compile-time constants `ALL_CAPS`; `constexpr` functions stay `camelCase`. Protocol names, operators, aliases such as `mint` and `iint`, template parameters such as `T` and `MOD`, and constructor size parameters such as `N` and `M` are exceptions.
- Use the same name for the same concept everywhere: `n` size, `m` second size or edge count, `l`/`r` range bounds, `i`/`j`/`k` indices, `u`/`v` vertices, `w` weight, `x`/`y` coordinates, `res` result.
- Braces on every `if`, `else`, loop and similar statement; ternaries excepted. Single-line blocks have spaces inside: `if (ok) { work(); }`.
- Closing braces: a multi-line block ends on the line of its last statement, `return res;}`. This applies to control blocks, function bodies and lambda bodies, and nests: `}}`. `else` starts a new line after `;}`. Exceptions: struct, class, union and enum bodies end with `};` on its own line; namespaces end with `} // namespace name` on its own line.
- One logical step per line: a short method may be one line; tightly coupled statements may share a line (`u = find(u); v = find(v);`); unrelated setup, mutation and branching are split. Keep loop headers and short braced conditionals compact.
- Group methods: construction (including `static` builders), access, mutation, queries, `friend` operators, then `friend` algorithms grouped by family; one blank line between groups; keep tiny overloads together. `16-poly.hpp` instead keeps each `static` builder inside its algorithm family. Never reorder data members to satisfy layout.
- Attach `*` and `&` to the name: `T *p`, `T &x`, `T &operator+=(const T &o)`.
- Casts: functional `T(x)` when unambiguous; `static_cast` otherwise; never hide a narrowing with a cast. Comparing a signed index with an unsigned size is acceptable when the bound is justified.
- Named lambdas: `auto f = [&](args) -> T { ... };` with the return type only when deduction is ambiguous. Recursive lambdas take `auto &&f` first and call `f(f, ...)`.
- Prefer `std::midpoint(l, r)` where its rounding fits. Constant factors first: `2 * n`, `32 * k`.
- Qualify standard names with `std::` except this closed list: `int8_t uint8_t int16_t uint16_t int32_t uint32_t int64_t uint64_t size_t ptrdiff_t string string_view istream ostream cin cout cerr pair tuple array bitset vector deque priority_queue queue stack map multimap unordered_map unordered_multimap set multiset unordered_set unordered_multiset abs max min gcd lcm reverse sort swap lower_bound upper_bound binary_search unique fill iota accumulate next prev`. Everything else, including `move`, `forward`, `exchange`, `midpoint`, traits and concepts, is qualified.

## Comments

- A struct, free function, `friend` algorithm or `static` builder carries at most two comment lines directly above it: the complexity line, and one line naming the domain and the no-answer representation, for example `// mod in [1, 2^32); inv asserts a unit; sqrt returns -1 when no root.`
- Inside a body, a comment is one line and appears only where the code's trick or invariant is not evident from the code itself. No restated contracts, no history, no references to other documents.
- A file may open with one comment line. No block comments.
- Everything else (full contracts, staleness and invalidation rules, sentinel policies, proofs, provenance) goes to the header's evidence document under `## Contracts`, one subsection per struct or free function (a type's `friend` algorithms and `static` builders go under the type's subsection or in their own subsections), linked from the inventory row.
- The validator rejects a verified header with more than two consecutive comment-only lines or with comment-only lines above 8% of its non-blank lines; one comment line is always allowed, so a one-function header keeps its complexity line. `02-Data Structures/00-monoids.hpp`, one complexity line per struct, is exempt from the 8% share.

## Complexity comments

- One comment line directly above every struct and free function except template and debug code: `// T: O(...), M: O(...)` for functions and simple structs, or `// S: O(...), U: O(...), Q: O(...), M: O(...)` for structures with setup, update and query. `NA` marks an inapplicable component.
- Symbols: `n`, `m`, `k`, `q` sizes and counts; `V`, `E` vertices and edges; `w` word width; `b` bit length; `L` string length; `S` alphabet size; `out` output size. Define any other symbol in the same comment. Write multiplication explicitly (`n * m`) and functions with parentheses (`log(n)`, `sqrt(n)`).
- State the worst case. Label `amortized` or `expected` bounds. Give conditions instead of `O(a) or O(b)`.
- `M` is stored data for a struct and auxiliary space for a function, excluding caller-owned inputs. Add workspace, cache or output terms when they exceed the stored bound: `M: O(n) (workspace O(n^2))`.
- A method, `friend` algorithm or `static` builder gets its own comment only when its bound differs from the struct comment. ISA kernels need no comment; the public dispatcher's bound covers them. For big integers distinguish operation counts from bit complexity.
