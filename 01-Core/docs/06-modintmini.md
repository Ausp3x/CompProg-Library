# C16: independent modular integer minis

`06-modintmini.hpp` contains four independently copyable structs: `ModIntMini<MOD>`, `ModInt64Mini<MOD>`, `DynModIntMini<ID = 0>` and `DynModInt64Mini<ID = 0>`. They complement the four full types in `05-modint.hpp`; the distinct alias `mintmini = ModIntMini<998244353>` coexists with the full `mint`.

## Domain and public contract

The 32-bit types support every modulus `1 <= m <= UINT32_MAX`; the 64-bit types support every modulus `1 <= m <= UINT64_MAX`. Values store one canonical residue `n` in `[0, m)`, with `Word` equal to `uint` or `ulng`. Do not mutate implementation state; direct writes to `n` must preserve its canonical representation. `red` and `powerMontgomery` are implementation-only helpers, not part of the shared public API: `red(x)` assumes a canonical-product dividend `x < mod()^2`, and does not promise arbitrary double-width reduction. Arithmetic has the same semantics as the full types on the shared API.

| Operation | Contract |
|---|---|
| Default/native integer construction | Default zero; normalize every native signed/unsigned integer, including `bool`, signed minima and signed/unsigned 128-bit values. Floating-point construction is rejected. Ordinary assignment through a constructed temporary is supported. |
| `mod()`, `val()`, `raw(a)` | Modulus, canonical residue, and construction from a canonical `Word`. `raw` requires `a < mod()` and checks this in assertion builds. |
| Unary `+ -`, binary/compound `+ - * /` | Exact modular arithmetic across the full modulus range. Division requires a unit denominator. Compound self-aliasing is supported. Integer operands convert through the constructor. |
| `==`, `!=` | Equality of canonical representatives of the same type; C++20 supplies `!=` from `==`. No cross-type implicit conversion. |
| `tryInv(a, out)`, `inv(a)` | Extended Euclid. `tryInv` returns false for nonunits and leaves `out` unchanged; `a`/`out` may alias. `inv` asserts the unit precondition. Composite moduli are supported. |
| `pow(a, e)` | Every native integer exponent width, including signed/unsigned 128 bits. Negative exponents require a unit base. The unsigned magnitude conversion handles signed minima without overflow. |
| Dynamic `setMod(m)` | Requires `m > 0`; constant-time setup. Default modulus is `998244353`. State is independent for every exact ID/width, separate from all full types. |

Hidden friends are called through argument-dependent lookup: `inv(a)`, `tryInv(a, out)` and `pow(a, e)`. All static-type arithmetic, constructors, accessors and inverse/power operations support constant evaluation. Dynamic types do not promise constexpr evaluation. For modulus one, the only value is zero, its inverse is zero, and `pow(0, 0) == 0`; these match the full types' zero-ring convention.

Every dynamic `setMod`, even with the previous modulus, invalidates existing values and derived caches. Stale values may only be discarded or overwritten. There are no generation tags or runtime checks for stale use. Setup and arithmetic follow the library's single-threaded baseline; concurrently mutating a modulus is outside the contract. Inline `constinit` state is shared safely across translation units and initialized before dynamic global initialization; ordering between user-defined global setup calls still matters.

Omitted APIs are deliberate mini reductions: primality flags/detection, prime/composite roots, batch inversion, arbitrary-length decimal streams, integer casts, truthiness/`!`, ordering, increment/decrement and compatibility spellings such as `init`. Use `val()` to pass canonical residues to ordinary output or another exact type. These omissions do not reduce the arithmetic domain or silently change division semantics.

## Independent copy and algorithms

Each struct contains its own normalization, inverse and power helpers. There is no dependency on another mini, a full implementation, a shared detail namespace, Barrett/Montgomery headers or a common base. The installed header includes `01-template.hpp` for the standard headers and shared aliases. When copying one struct independently, supply these standard headers and exact aliases:

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

Unused headers/aliases may be removed per selected struct. GNU C++20 and the repository's GCC14+ baseline still apply; the GNU `noipa` attribute on the 64-bit power helper preserves the measured generic reduction kernel. Standard includes and width aliases are the only copy-paste prerequisites.

Construction, ordinary arithmetic, raw/access/equality and setup use constant space/time under fixed machine-word arithmetic. Inverse/division cost `O(log(m))`; powers cost `O(1 + log(|e| + 1))`, with an additional `O(log(m))` inverse for negative exponents. Each value stores four or eight bytes. Dynamic contexts have one modulus and, for 32 bits, one 64-bit reciprocal per exact ID; no heap allocation or growing cache exists. Powers use constant workspace, including Montgomery setup.

- Static 32/64-bit products use exact double-width multiplication followed by constant-modulus remainder, allowing compiler specialization.
- Dynamic 32-bit products use a mask for powers of two and a compact reciprocal reduction otherwise. For `R = 2^64`, the stored reciprocal is `ceil(R/m)`, with intentional unsigned wrap to zero for `m = 1`. For canonical `a,b`, `x = a*b < m^2 < R`; the high product gives a quotient equal to `floor(x/m)` or one greater. Its product with `m` fits 64 bits, and one correction produces the canonical remainder. The zero ring has only `x = 0`.
- Dynamic 64-bit products use a mask for powers of two, one Mersenne fold for `m = 2^61 - 1`, and exact unsigned 128-bit remainder otherwise. For the Mersenne path, canonical products are below `m^2`, so the folded sum is below `2*m` and one subtraction suffices.
- Both 64-bit structs contain a compact full-width Montgomery power helper. It supports every odd modulus without requiring `m < 2^63`: it retains the high carry when adding the conceptual 128-bit REDC terms and makes one conditional subtraction. Canonical products and conversion products satisfy `x < m*2^64`. Five Newton updates from the odd modulus yield its inverse modulo `2^64`; unsigned negation and wrap are intentional. Odd powers dispatch at magnitude `512`, or `65536` for static Mersenne61. Dynamic Mersenne61 retains its cheaper single-fold product at every exponent length. Small/even/constexpr powers use ordinary multiplication; the zero ring returns immediately. Per-power setup and conversion are included in measurements.
- Addition/subtraction compare against the modulus before arithmetic, avoiding full-word overflow. The Euclidean remainder/cofactor invariant bounds coefficients by the modulus; signed 64-bit cofactors suffice for 32-bit moduli and signed 128-bit cofactors suffice for 64-bit moduli. A failed inverse does not assign to the output.

The repeated code is intentional: each struct remains independently copyable. Mini scope removes uncommon operations before reducing arithmetic quality. These scalar kernels need no ISA-specific code; compiler-generated scalar/AVX2 differences are measured separately.

## Verification and performance evidence

The [test entry](<../../96-Local Testing/01-Core/06-modintmini_tester.py>) owns all four structs. Full verification on 2026-09-27 passed **176,384 independent Python integer oracle cases per configuration**, plus the following API/integration fixtures, in optimized `-O3 -DNDEBUG`, checked `-O1 -D_GLIBCXX_ASSERTIONS`, and ASan/UBSan scalar builds. These structs have no ISA-specific branches; aggregate integration and the benchmark also exercise AVX2/BMI2 compilation.

| Feature | Verification |
|---|---|
| Construction, assignment, raw/access, equality | All native signed minima, unsigned maxima, 128-bit normalization, booleans, default/copy/move/self assignment, both integer operand orders; independent Python remainder |
| Arithmetic and unit inverse/division | Exhaustive static rings 1–16, dynamic rings 1–48, boundary/common moduli, 512 random pairs per large modulus, 300 arbitrary dynamic moduli per width; exact Python arithmetic/inverse plus full-type agreement |
| Aliasing and failure behavior | Compound operations return the same reference, self-arithmetic, aliased inverse output, unchanged nonunit outputs, zero-ring conventions |
| Powers and constexpr | Native signed/unsigned exponents through 128 bits including minima/maxima, zero, negative units, Montgomery/Mersenne threshold neighbors; static assertions and Python `pow` |
| Dynamic lifecycle | Default setup, changed/same-modulus reset with stale values overwritten before use, IDs/widths/full-mini independence, default/custom globals in both translation-unit link orders |
| Preconditions and standalone use | 18 checked assertion deaths, both zero static moduli rejected even under `NDEBUG`, each of four extracted structs compiled/run independently with only standard includes/aliases |

Quick retains every API fixture with dynamic rings through 24 and 64 random pairs per large modulus, using optimized and checked configurations. Stress extends dynamic rings through 96, random pairs to 4096 and arbitrary moduli to 3000 per width with the full configuration matrix; stress mode is available but no stress run is claimed. The full run used GCC 16.2.1 (20260810), CPython 3.14.7 and the Intel Core i9-11900H host. GCC14, Python3.10 and PyPy execution were unavailable; no run on those versions is claimed.

```bash
python3 '96-Local Testing/01-Core/06-modintmini_tester.py' --mode full --seed 20260927
```

The final full suite ran outside the process-traced sandbox after LeakSanitizer explicitly refused that environment; address, undefined-behavior and leak checks passed. An additional independent UBSan check passed 100,000 random 32/64-bit products and 128-bit exponent powers; the committed suite includes systematic versions of those boundary classes. The final persistent suite was rerun after tightening its extracted-struct preamble to exactly the five documented standard headers and aliases, without `using namespace std`; all three configurations passed in 76.78 seconds. Each documented minimal copy recipe was also separately compiled and run without `using namespace std` and with `-Wall -Wextra -Werror`. The migration snapshot mini-header SHA256 was `74c80da300e6fc5fca036bf5e1d250d11ed3330f3276e10e1aa48879b3e84985`; the later maintenance hash is recorded below.

The [benchmark entry](<../../96-Local Testing/01-Core/06-modintmini_benchmark.py>) compares actual mini/full public arithmetic with native double-width remainder and a mini multiplication-only power candidate. The historical [JSONL record](<../../96-Local Testing/01-Core/06-modintmini_benchmark.jsonl>) contains **3,552 checked measurements** from that migration snapshot in scalar and AVX2/BMI2 builds and captures source hashes, compiler commands, host, seed, workload and medians. All benchmark outputs passed their exact checks.

```bash
python3 '96-Local Testing/01-Core/06-modintmini_benchmark.py' --seed 20260927 --repetitions 5 --milliseconds 1 --output /tmp/modintmini-benchmark.jsonl
```

Each workload warms up by doubling its iteration count to at least 1 ms, then records the median of five repetitions. Both scalar `-mno-avx2 -mno-bmi2` and AVX2/BMI2 `-mavx2 -mbmi2 -mno-avx512f` builds use GNU++20, `-O3 -DNDEBUG`. Ordinary independent products use uniform and near-modulus inputs at lengths 1, 8, 256 and 4096; dependent chains use unit factors at lengths 8 and 256. Eight-base powers cover zero, one, short exponents, 511/512/513, 65535/65536/65537, and sparse/dense 128-bit exponents. Static/dynamic domains include one, powers of two, a common prime, full-word prime/composite boundaries and Mersenne61. Input allocation and dynamic setup are excluded from timing; local Montgomery setup/conversions are included per power. Every result is checked against exact remainder arithmetic before timing.

Representative scalar medians are in microseconds; power rows cover all eight bases. `p64 = 18446744073709551557`.

| Workload | Native remainder | Mini | Full |
|---|---|---|---|
| Dynamic 32-bit modulus 1024, 256-factor chain | 1.387 | 0.226 | 0.298 |
| Dynamic 32-bit modulus 998244353, 256-factor chain | 2.068 | 1.543 | 1.597 |
| Static p64, exponent 512 | 0.289 | 0.252 | 0.255 |
| Static p64, exponent `2^128 - 1` | 5.795 | 3.226 | 3.304 |
| Dynamic Mersenne61, exponent 65536 | 0.519 | 0.286 | 0.394 |
| Dynamic Mersenne61, exponent `2^128 - 1` | 5.621 | 3.051 | 3.129 |

These measurements justify retaining the compact reciprocal/mask paths and Montgomery for long ordinary odd-modulus powers. They also distinguish the mini's cheaper single-fold dynamic Mersenne61 path from the full reducer: at exponent 65536 the multiplication-only candidate measured 0.268 µs versus the mini public call's 0.286 µs and the full Montgomery call's 0.394 µs. The AVX2/BMI2 build preserves that choice (0.230, 0.247 and 0.360 µs respectively). Ordinary products and short powers retain compact native reduction where setup would not pay. The zero-ring shortcut avoids unnecessary loops/setup; raw identity construction avoids normalization of a known canonical one.

Other implementation/test compiler jobs were paused during the final measurements, but this shared host was not frequency-isolated. Individual mini/full rows can vary by several percent, and some full rows are faster. The evidence supports the selected compact backends, not a claim that every mini operation matches or beats the full type on every host. The separate C16 benchmark record does not rewrite historical C03 measurements.

## Maintenance review — 2026-09-27

The P005/C16 maintenance review checked the current co-located layout against the revised C++ guide and [Core migration record](../../00-Guidelines/22-core-migration.md). All four structs remain independently copyable, with no sibling/full/base/detail/reduction-header dependency. Their full unsigned modulus domains, compact backends, arithmetic, public names, `mintmini`, and state/member ordering are unchanged. Header changes are whitespace only: logical setup/construction separators and named-lambda capture spacing. The benchmark support also received whitespace-only lambda/control-block formatting. Historical timings and their source hashes are preserved byte-for-byte; no performance rerun is claimed or required for these edits.

Persistent tests now enforce the reduced contract: floating-point constructors/exponents, implicit cross-family/width/ID conversions, integer casts, truthiness/`!`, ordering, increments/decrements, primality/roots, batch inversion, streams and `init` remain absent. Exact word widths and `mintmini` identity are checked. Each extracted struct still compiles without `using namespace std`, using exactly the five documented standard headers and aliases; those isolated programs now additionally exercise signed-minimum and unsigned-maximum 128-bit construction and powers. Generated C++ fixture code follows the current bracing/spacing defaults. These checks supplement the independent Python oracle and full/mini agreement rather than substituting for them.

Fresh verification used the full command above with seed 20260927 on GCC 16.2.1 and CPython 3.14.7: all three configurations passed **176,384 exact oracle cases each**, all 18 assertion deaths, both static zero-modulus rejections, all four isolated copies and cross-TU initialization in both link orders. The completed run took 78.57 seconds; ASan/UBSan and leak checking passed outside the known process-tracing restriction. The Python tester also parses under Python 3.10 grammar. Both scalar and AVX2/BMI2 optimized benchmark builds passed without running timings. No C16 verification gap remains.

The maintained mini header SHA256 is `2c90b006ca8cd4c5680ac474ed05f28507df5bcad1ec9b728cf76a063e365009`. Existing numbering, ownership, aliases, aggregate membership and shared maps remain unchanged; one integration owner reviewed those records. The full-family maintenance result is recorded in [C03 evidence](05-modint.md).

## Re-audit — 2026-10-07 (P005)

The confirmed `/reaudit-review` findings for this header were all style: multi-line blocks closing `; }` instead of `;}` (constructor branches, Montgomery dispatch, Mersenne fold), the `red` lambda in both 64-bit `powerMontgomery` helpers closing on its own `};` line with a spaced capture list, and no `T:/M:` line above those helpers although their bound differs from the struct's `O(1)`. All three are fixed: every multi-line block now ends `;}`/`}}`, the lambda is `auto red = [m, v](ulll x) -> Word { ... ;};`, and `// T: O(1 + log(e + 1)), M: O(1); odd m, canonical a.` sits directly above each helper. No algorithm, public name, domain or result changed; the mini remains independent of the full header and of the new full-type `ModInt61` fast path, as documented in `80-notes.md`. The 2026-10-07 researcher sweep of minimal modints (KACTL, ACL, Nyaan, maspypy, ei1333, ecnerwala, Benq, cp-algorithms, OI Wiki) found no operation outside the documented reductions; `!=` comes from `==` in C++20 and mixed integer operands convert through the constructor.

The tester and benchmark C++ sources were normalized to the same closing-brace rule (lambda closes joined to their last statement; the static oracle dispatch now ends `}}}` before its `#undef` lines). `03-consistency.py --braces` reports no violation for the header, tester or benchmark source, and the validator now enforces the rule on them because the row and package are verified. A validator defect found during this pass (a `template<...> requires (...)` clause on the line before `struct X {` made the checker treat the struct body as a block, since it joined lines without a separator) was fixed by inserting a space at each line break; that change is recorded in the C03 evidence.

Full verification before and after the edits used the command above with seed 20260927 on GCC 16.2.1 and CPython 3.14.7: **176,384 exact oracle cases per configuration** in optimized, checked and ASan/UBSan scalar builds, all 18 assertion deaths, both zero-modulus rejections, all four extracted copies and both translation-unit link orders; 86.69 s before, 85.61 s after. The AVX2/BMI2 optimized benchmark build also compiles. No timing was repeated: the header's generated code is unchanged apart from comments and whitespace, and the historical JSONL record stays byte-for-byte as the mini's measurement evidence.

## Research and provenance

This implementation is independently written; no external source code or license-bearing text was copied. The compact arithmetic follows the algorithms already justified in [C02 reduction evidence](p004-reduction.md) and [C03 full modular evidence](05-modint.md), rechecked for independent structs and full unsigned boundaries.

| Inspected reference | Scope and use |
|---|---|
| [AtCoder modint documentation](https://raw.githubusercontent.com/atcoder/ac-library/master/document_en/modint.md), saved source inspected 2026-09-27 | Canonical construction, raw, arithmetic, inverse and dynamic-context API comparison. ACL's narrower dynamic modulus contract does not justify the full-word domains here. |
| [AtCoder internal math](https://raw.githubusercontent.com/atcoder/ac-library/master/atcoder/internal_math.hpp), `barrett` and `inv_gcd`, saved source inspected 2026-09-27 | Reciprocal quotient/correction and Euclidean cofactor invariants. ACL is CC0; no code copied. |
| [cp-algorithms: Modular Multiplicative Inverse](https://cp-algorithms.com/algebra/module-inverse.html), extended-Euclidean and prime-only distinction, saved source inspected 2026-09-27 | Composite-modulus unit inverse and explicit rejection of prime-only inversion shortcuts. |
| Current `03-barrett.hpp`, `04-montgomery.hpp`, and `05-modint.hpp` | Full-width carry argument, product domains, constant-time setup, power dispatch and matching semantics. These are reference implementations, not dependencies of a copied mini. |

No online submission or acceptance is claimed. The source review establishes the stated algorithm choices; finite tests and benchmarks do not establish a universal fastest implementation.
