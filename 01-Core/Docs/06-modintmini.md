# 06-modintmini.hpp — Evidence

`06-modintmini.hpp` contains four independently copyable structs: `ModIntMini<MOD>`, `ModInt64Mini<MOD>`, `DynModIntMini<ID = 0>` and `DynModInt64Mini<ID = 0>`. They complement the six full types in `05-modint.hpp`; the distinct alias `mintmini = ModIntMini<998244353>` coexists with the full `mint`.

## Contracts

### ModIntMini, ModInt64Mini, DynModIntMini, DynModInt64Mini

The 32-bit types support every modulus `1 <= m <= UINT32_MAX` and the 64-bit types every modulus `1 <= m <= UINT64_MAX`. A value stores one canonical residue `n` in `[0, m)`, with `Word` equal to `uint` or `ulng`. Implementation state must not be mutated, and a direct write to `n` must keep it canonical. `red` is an internal helper, not shared public API: `red(x)` assumes a product dividend `x < mod()^2` and promises no arbitrary double-width reduction. On the shared API the arithmetic has the full types' semantics.

| Operation | Contract |
|---|---|
| Default/native integer construction | Default zero; normalize every native signed/unsigned integer, including `bool`, signed minima and signed/unsigned 128-bit values. Floating-point construction is rejected. Ordinary assignment through a constructed temporary is supported. |
| `mod()`, `val()`, `raw(a)` | Modulus, canonical residue, and construction from a canonical `Word`. `raw` requires `a < mod()` and checks this in assertion builds. |
| Unary `+ -`, binary/compound `+ - * /` | Exact modular arithmetic across the full modulus range. Division requires a unit denominator. Compound self-aliasing is supported. Integer operands convert through the constructor. |
| `==`, `!=` | Equality of canonical representatives of the same type; C++20 supplies `!=` from `==`. No cross-type implicit conversion. |
| `tryInv(a, out)`, `inv(a)` | Extended Euclid. `tryInv` returns false for nonunits and leaves `out` unchanged; `a`/`out` may alias. `inv` asserts the unit precondition. Composite moduli are supported. |
| `pow(a, e)` | Every native integer exponent width, including signed/unsigned 128 bits. Negative exponents require a unit base. The unsigned magnitude conversion handles signed minima without overflow. |
| Dynamic `setMod(m)` | Requires `m > 0`; constant-time setup. Default modulus is `998244353`. State is independent for every exact ID/width, separate from all full types. |

`inv(a)`, `tryInv(a, out)` and `pow(a, e)` are hidden friends found by argument-dependent lookup. Every static-type constructor, accessor, arithmetic, inverse and power operation supports constant evaluation; dynamic types promise none. Under modulus one the only value is zero, its inverse is zero and `pow(0, 0) == 0`, the full types' zero-ring convention.

Every dynamic `setMod`, even with the previous modulus, invalidates existing values and derived caches; stale values may only be discarded or overwritten, and nothing tags or checks them. Setup and arithmetic follow the library's single-threaded baseline, so changing a modulus concurrently is outside the contract. Inline `constinit` state is shared safely across translation units and initialized before dynamic global initialization; the order of user-defined global setup calls still matters.

Not provided until P255 brings the minis to parity with the full row ([00-notes.md](00-notes.md#reduction-and-modular-types)): the Montgomery-form types, primality flags and detection, prime and composite roots, batch inversion, arbitrary-length decimal streams, integer casts, truthiness and `!`, ordering, and increment and decrement. `val()` passes canonical residues to ordinary output or another exact type. The omissions narrow neither the arithmetic domain nor the division semantics.

### Independent Copy

Each struct holds its own normalization, inverse and power helpers and depends on no other mini, full implementation, shared detail namespace, Barrett or Montgomery header or common base. The installed header includes `01-template.hpp` for the standard headers and shared aliases; a struct copied alone needs these standard headers and exact aliases:

```cpp
#include <cassert>
#include <bit>
#include <cstdint>
#include <type_traits>
#include <utility>
using uint = uint32_t;
using lng = int64_t;
using ulng = uint64_t;
using lll = __int128_t;
using ulll = __uint128_t;
```

Headers and aliases a selected struct does not use may be removed. GNU C++20 and the repository's GCC 14 baseline still apply.

### Algorithms and Cost

Construction, ordinary arithmetic, `raw`, access, equality and setup take O(1) time and space in machine words. Inverse and division cost `O(log(m))`; powers cost `O(1 + log(|e| + 1))`, plus an `O(log(m))` inverse for a negative exponent, in constant workspace. A value is four or eight bytes. A dynamic context holds one modulus and, for 32 bits, one 64-bit reciprocal per exact ID; there is no heap allocation or growing cache.

- Static 32- and 64-bit products are an exact double-width multiplication and a constant-modulus remainder, which the compiler specializes.
- Dynamic 32-bit products mask for a power of two and otherwise use a compact reciprocal reduction. With `R = 2^64` the stored reciprocal is `ceil(R/m)`, wrapping to zero by design for `m = 1`. For canonical `a, b`, `x = a*b < m^2 < R`, and the high product gives `floor(x/m)` or one more; its product with `m` fits 64 bits, and one correction gives the canonical remainder. The zero ring has only `x = 0`.
- Dynamic 64-bit products mask for a power of two, fold once for `m = 2^61 - 1` and otherwise take the exact unsigned 128-bit remainder. On the Mersenne path canonical products are below `m^2`, so the folded sum is below `2m` and one subtraction suffices.
- Powers use the struct's own multiplication at every exponent; the zero ring returns at once. The 64-bit structs have no per-call Montgomery helper: on 4096 distinct bases its per-call setup and REDC correction branch made it slower than the native remainder from about `2^63` upward (1.7x at `2^64-59`) and about even below. The full type's power dispatch is in [05-modint.md](05-modint.md#pow).
- Addition and subtraction compare with the modulus before the arithmetic, so a full word cannot overflow. Euclid's remainder and cofactor invariant bounds the coefficients by the modulus: signed 64-bit cofactors suffice for 32-bit moduli and signed 128-bit ones for 64-bit moduli. A failed inverse does not write the output.

The code is repeated on purpose, so that each struct stays independently copyable. The scalar kernels need no ISA-specific code; compiler-generated scalar and AVX2 differences are measured separately.

## Feature-to-Test Map

The [test entry](<../../96-Local Testing/01-Core/06-modintmini_tester.py>) checks all four structs against an independent Python integer oracle in the optimized `-O3 -DNDEBUG`, checked `-O1 -D_GLIBCXX_ASSERTIONS` and ASan/UBSan scalar builds. The structs have no ISA-specific branches; aggregate integration and the benchmark also compile them with AVX2 and BMI2.

| Feature | Verification |
|---|---|
| Construction, assignment, raw/access, equality | All native signed minima, unsigned maxima, 128-bit normalization, booleans, default/copy/move/self assignment, both integer operand orders; independent Python remainder |
| Arithmetic and unit inverse/division | Exhaustive static rings 1–16, dynamic rings 1–48, boundary/common moduli, 512 random pairs per large modulus, 300 arbitrary dynamic moduli per width; exact Python arithmetic/inverse plus full-type agreement |
| Aliasing and failure behavior | Compound operations return the same reference, self-arithmetic, aliased inverse output, unchanged nonunit outputs, zero-ring conventions |
| Powers and constexpr | Native signed/unsigned exponents through 128 bits including minima/maxima, zero, negative units, the neighbors 511/512/513 and 65535/65536/65537; static assertions and Python `pow` |
| Dynamic lifecycle | Default setup, changed/same-modulus reset with stale values overwritten before use, IDs/widths/full-mini independence, default/custom globals in both translation-unit link orders |
| Preconditions and standalone use | 18 checked assertion deaths, both zero static moduli rejected even under `NDEBUG`, each of four extracted structs compiled/run independently with only standard includes/aliases |
| Reduced contract | Compile-time absence of floating-point constructors/exponents, implicit cross-family/width/ID conversions, integer casts, truthiness/`!`, ordering, increments/decrements, primality/roots, batch inversion and streams; exact word widths and `mintmini` identity |

Quick keeps every API fixture, with dynamic rings through 24 and 64 random pairs per large modulus, in the optimized and checked configurations. Stress extends dynamic rings through 96, random pairs to 4096 and arbitrary moduli to 3000 per width, in every configuration. Each extracted struct compiles with `-Wall -Wextra -Werror`, without `using namespace std`, on exactly the five documented standard headers and the aliases.

## Commands and Results

Latest runs, 2026-11-21 and 2026-11-22, in P005's `/finish C03` chain (Intel Core i9-11900H, GCC 16.2.1, CPython 3.14.8 and 3.10.22, seed 1; commands and conditions in [05-modint.md](05-modint.md#commands-and-results)):

```bash
python3 '96-Local Testing/01-Core/06-modintmini_tester.py' --mode full --seed 1                # PASS, 3 configurations, 176,434 oracle and full-against-mini cases each, 54 s, peak 1022 MB
python3.10 '96-Local Testing/01-Core/06-modintmini_tester.py' --mode quick --seed 1            # PASS under CPython 3.10.22, 3 s
python3 '96-Local Testing/01-Core/06-modintmini_tester.py' --mode quick --seed 1               # PASS on the committed tree, 2 configurations, 4 s
```

Montgomery helper removal, 2026-10-12 (the 64-bit structs' per-call Montgomery power helper dropped; GCC 16.2.1 and GCC 14.4.1, CPython 3.14.7, Intel Core i9-11900H), the latest run of each command:

```bash
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/06-modintmini.hpp '96-Local Testing/01-Core/06-modintmini_benchmark.cpp'   # no violations
CXX=g++-14 python3 '96-Local Testing/01-Core/06-modintmini_tester.py' --mode full --seed 20261012      # PASS, 3 configurations, 176,644 cases each, 85 s, peak 756 MB
python3 '96-Local Testing/02-integration.py'                                                           # PASS, 136 headers, scalar/AVX2 multi-TU, workspace
python3 '96-Local Testing/03-consistency.py'                                                           # no errors
python3 '96-Local Testing/01-Core/06-modintmini_benchmark.py' --seed 20261012 --repetitions 5 --milliseconds 2 --output '96-Local Testing/01-Core/06-modintmini_benchmark.jsonl'   # 3,552 measurements, all verified
```

Full mode covers the optimized, checked and ASan/UBSan scalar builds, the assertion deaths, both zero-modulus rejections under `NDEBUG`, all four individually copied structs and both translation-unit link orders; the 2026-10-12 durations ran beside the modint suites and include the quick preflight.

Library-wide runs of 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, run beside the other suites, build cache emptied before each cold run), all PASS:

| Command | Compiler | This suite | Memory peak |
|---|---|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` | GCC 16.2.1 | preflight quick peak 998 MB; optimized-scalar 1.5 s compile + 0.8 s run, checked-scalar 1.8 s + 1.1 s, sanitized-scalar 88.5 s + 1.9 s | 723 MB |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | GCC 14.4.1 | preflight quick peak 1009 MB; optimized-scalar 1.5 s + 0.7 s, checked-scalar 1.5 s + 0.7 s, sanitized-scalar 77.8 s + 1.6 s | 759 MB |
| `python3 '96-Local Testing/01-run.py' --mode quick` | GCC 16.2.1 | optimized-scalar 110.4 s + 0.2 s, checked-scalar 92.1 s + 0.4 s | 989 MB |
| `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | GCC 16.2.1 | preflight quick peak 1000 MB; optimized-scalar 0.5 s + 1.0 s, checked-scalar 0.5 s + 1.1 s, sanitized-scalar 38.1 s + 2.7 s | 884 MB |

## Benchmarks

The [benchmark entry](<../../96-Local Testing/01-Core/06-modintmini_benchmark.py>) compares the mini and full public arithmetic with the native double-width remainder and a mini multiplication-only power candidate. `96-Local Testing/01-Core/06-modintmini_benchmark.jsonl` (2026-10-12, seed 20261012) holds **3,552 checked measurements** in scalar and AVX2/BMI2 builds with source hashes, compiler commands, host, seed, workload and medians.

Each workload warms up by doubling its iteration count to at least 2 ms, then records the median of five repetitions. The scalar `-mno-avx2 -mno-bmi2` and AVX2/BMI2 `-mavx2 -mbmi2 -mno-avx512f` builds use GNU++20 and `-O3 -DNDEBUG`. Independent products use uniform and near-modulus inputs at lengths 1, 8, 256 and 4096; dependent chains use unit factors at lengths 8 and 256. Powers run 4096 distinct random bases per pass over zero, one, short exponents, 511/512/513, 65535/65536/65537 and sparse and dense 128-bit exponents. Static and dynamic moduli include one, powers of two, a common prime, full-word prime and composite boundaries and Mersenne61. Input allocation and dynamic setup are not timed, and every result is checked against exact remainder arithmetic before timing.

Representative scalar medians; chains in microseconds, powers in nanoseconds per power. `p64 = 18446744073709551557`. The Full column is the pre-C03 header of 2026-10-12, not the current `05-modint.hpp`.

| Workload | Native remainder | Mini | Full (pre-C03 header, 2026-10-12) |
|---|---|---|---|
| Dynamic 32-bit modulus 1024, 256-factor chain | 1.544 | 0.267 | 0.291 |
| Dynamic 32-bit modulus 998244353, 256-factor chain | 1.422 | 0.933 | 0.872 |
| Dynamic Mersenne61, 256-factor chain | 1.437 | 0.635 | 0.562 |
| Static `ModInt64Mini<998244353>`, exponent 512 / 2^128-1 | 34.2 / 767 | 33.7 / 791 | 20.9 / 407 |
| Static p64, exponent 512 / 2^128-1 | 34.7 / 821 | 34.3 / 812 | 34.4 / 790 |
| Dynamic Mersenne61, exponent 512 / 65536 | 33.4 / 64.9 | 16.9 / 33.0 | 19.9 / 31.4 |
| Dynamic p64, exponent 512 / 2^128-1 | 35.5 / 765 | 37.4 / 998 | 37.3 / 758 |

These measurements justify the compact reciprocal and mask paths and the dynamic single-fold Mersenne61 product, which matched the full type within noise. The minis' powers use the native remainder; where that full type's cached Montgomery context won (998244353 at exponent 512: 20.9 against 33.7 ns), the gain was the full type's. Single rows vary by several percent on this shared host without frequency isolation, and some full rows are faster: the evidence supports the chosen compact backends, not a claim that every mini operation beats the full type.

## Sources

Independently written; no external code or license-bearing text was copied. The compact arithmetic follows the algorithms justified in the C02 reduction evidence ([03-barrett.md](03-barrett.md), [04-montgomery.md](04-montgomery.md)) and the [C03 full modular evidence](05-modint.md), rechecked for independent structs and full unsigned boundaries.

| Inspected reference | Scope and use |
|---|---|
| [AtCoder modint documentation](https://raw.githubusercontent.com/atcoder/ac-library/master/document_en/modint.md), saved source inspected 2026-09-27 | Canonical construction, raw, arithmetic, inverse and dynamic-context API comparison. ACL's narrower dynamic modulus contract does not justify the full-word domains here. |
| [AtCoder internal math](https://raw.githubusercontent.com/atcoder/ac-library/master/atcoder/internal_math.hpp), `barrett` and `inv_gcd`, saved source inspected 2026-09-27 | Reciprocal quotient/correction and Euclidean cofactor invariants. ACL is CC0; no code copied. |
| [cp-algorithms: Modular Multiplicative Inverse](https://cp-algorithms.com/algebra/module-inverse.html), extended-Euclidean and prime-only distinction, saved source inspected 2026-09-27 | Composite-modulus unit inverse and explicit rejection of prime-only inversion shortcuts. |
| Current `03-barrett.hpp`, `04-montgomery.hpp`, and `05-modint.hpp` | Full-width carry argument, product domains, constant-time setup, power dispatch and matching semantics. These are reference implementations, not dependencies of a copied mini. |

The 2026-10-07 sweep of minimal modints (KACTL, ACL, Nyaan, maspypy, ei1333, ecnerwala, Benq, cp-algorithms, OI Wiki) found no operation outside the documented reductions; see [00-sources.md](00-sources.md) and [00-notes.md](00-notes.md).

## Limits and Handoffs

- Python 3.10: quick mode passed under CPython 3.10.22 on 2026-11-21 ([05-modint.md](05-modint.md#commands-and-results)); full and stress were not run.
- The mini stays independent of the full type's `ModInt61` fold path.
- C16 reopens after C03 is verified (P005 plan note; user decisions 2026-11-19 and 2026-11-21) to take the scalar wins of the rewritten full type; until then the Full column of Benchmarks is measured against the pre-C03 header, and the mini's speed ratio to the current full type is not recorded.
- Verified before the mutation rule of 2026-11-03; no record yet.

## Aliases

Search aliases (P263, grab --find): 取模类, 自动取模, 乘法逆元, 快速幂, 动态模数.

## History

- 2026-09-27: original C16 verification, full suite passed (176,384 cases per configuration); benchmark record.
- 2026-09-27: maintenance review, reduced-contract absence checks added, full suite passed.
- 2026-10-07: re-audit, 3 style findings fixed (closers, lambda form, missing complexity line), full suite passed on g++.
- 2026-10-07 (run): full (seed 20260927, 176,384 cases per configuration, 85.61 s) passed; P005 stress round passed 1,044,573 cases per configuration.
- 2026-10-08: restyle (header unchanged), full suite passed on g++ and g++-14, integration and consistency passed.
- 2026-10-12: the 64-bit structs' per-call Montgomery power helper removed (slower than the native remainder from about `2^63` upward on distinct bases, even below), the benchmark moved to 4096 distinct bases; full suites passed on g++ and g++-14 (176,644 cases per configuration; g++ 78 s, peak 723 MB).
- 2026-11-07: P263 follow-up added Chinese search aliases (`grab --find`), Simplified plus Taiwan or Hong Kong terms where they differ; no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared ccache `Builder` in `_00_cpp_test_runner.py`, up to three at once within the memory cap, 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-21: full (seed 1, 176,434 cases per configuration) and quick under CPython 3.10.22 passed as a consumer in P005's `/finish C03` chain ([05-modint.md](05-modint.md#commands-and-results)); quick passed on the committed tree on 2026-11-22.
