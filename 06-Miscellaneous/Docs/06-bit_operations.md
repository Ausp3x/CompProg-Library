# 06-bit_operations.hpp — evidence

`06-bit_operations.hpp` provides `BitOps<U>` (default `ulng`) for unsigned scalar words. It delegates standard operations to C++20 `<bit>` and adds boundary contracts, individual-bit manipulation, Gray codes and constant-time mask steps. Dynamic storage is Core Bitset; general enumerators are `12-enumeration.hpp`. Batch MI02, package P013; dependency P002 (template).

## Contracts

### BitOps

`U` must be an unqualified unsigned type supported by `std::popcount`: unsigned
char/short/int/long/long long and GNU unsigned 128-bit integers on this platform.
The shared aliases `uint`, `ulng`, `ulll` and `uint8_t`/`uint16_t` work. Signed
types, `bool`, cv-qualified types and character code-unit types such as `char8_t`
are rejected at compile time. Explicitly converting a signed value to a chosen
unsigned word gives its modulo-2^W representation; the API never performs an
implicit signed shift. `W` is the type's bit width, at most 128.

Every operation is `constexpr`, O(1) time and O(1) auxiliary memory for these
fixed scalar widths (`grayDecode` takes O(log(w)) shift-xor steps, at most 7). The compiler chooses ordinary scalar instructions; there
are no handwritten ISA kernels or target-specific flags.

| Operation | Contract |
|---|---|
| `count(x)`, `width(x)` | Number of one bits and significant bits; both zero at x=0. |
| `leadingZeros(x)`, `trailingZeros(x)` | Zero runs at either end; both W at x=0. |
| `first(x)`, `last(x)` | Lowest/highest set-bit position, or -1 at x=0. |
| `single(x)` | True exactly for a power of two; false at zero. |
| `lowBit(x)`, `floor(x)` | Lowest set-bit mask and highest power of two ≤x; both zero at x=0. |
| `ceil(x,out)` | Smallest power of two ≥x, treating x=0 as 1. False when it exceeds the word, leaving out unchanged. Input and output may alias. |
| `lowMask(k)` | Lowest k bits set; require 0≤k≤W, including the full word. |
| `test(x,i)`, `set(x,i,on=true)`, `flip(x,i)` | Read/set/clear/flip bit i; require 0≤i<W. Mutation helpers return a word and do not change their input. |
| `shiftLeft(x,k)`, `shiftRight(x,k)` | Logical shifts; require 0≤k≤W. A shift of W returns zero. Left shift discards bits leaving the word. |
| `rotateLeft(x,k)`, `rotateRight(x,k)` | All `int` counts, including INT_MIN/MAX; counts reduce modulo W and negative counts reverse direction. |
| `parity(x)` | Popcount modulo 2 as `bool`. |
| `grayCode(x)`, `grayDecode(x)` | Reflected binary Gray code `x ^ (x >> 1)` and its inverse (bit i of the decode is the xor of bits ≥ i); mutual inverses over the whole word. |
| `nextSupermask(sup,mask,full=~0)` | Require mask⊆full and mask⊆sup⊆full. Advance to the next larger set between mask and full, returning true; `sup == full` returns false unchanged. Start from `sup = mask` to visit all 2^popcount(full & ~mask) supersets in increasing order. |
| `prevSubmask(sub,mask)` | Require sub⊆mask. Advance to the next smaller submask, returning true; zero has no predecessor, returns false and stays zero. |
| `nextCombination(x)` | Advance to the smallest larger word with the same popcount. Zero and the last such word return false without mutation. |

The stepping operations charge O(1) per step, not O(1) for an entire traversal.
For example, all submasks take Θ(2^popcount(mask)) time, including zero:

```cpp
ulng sub = mask;
do { /* process sub, including zero */ } while (BitOps<>::prevSubmask(sub, mask));
```

Positions/count bounds and the subset condition are asserted preconditions.
They remain caller obligations under NDEBUG. Overflow of a power-of-two ceiling
and exhaustion of a mask sequence are valid results, reported by `false`.

Correctness: the standard count/scan/rotation operations have defined zero and modulo-width semantics; `ceil` guards the only domain where `std::bit_ceil` would overflow, and width-equal branches in masks/shifts avoid shifting by the type's width. Narrow operands promote to `int`: the largest 16-bit shifted intermediate is 65535×2^15, which fits, every shift count stays below the promoted width, and explicit conversions restore 8/16-bit wrap after addition and complement.

- `prevSubmask`: for nonzero `sub`, subtracting one clears its lowest set bit and fills the positions below; intersecting with `mask` gives the largest smaller submask. Testing zero first prevents cycling back to `mask`.
- `nextSupermask`: the free bits `rest = full & ~mask` form a counter. Adding one to `sup | ~rest` carries exactly through the free positions in increasing order; masking with `rest` and restoring `mask` gives the next set. The only wrap is all free bits set, the `sup == full` case checked first.
- `nextCombination`: adding `low` (the lowest set bit) carries through the lowest run of ones, putting its top one in the next free higher position; wrapping to zero means no larger result. Otherwise `next ^ x` identifies the changed run, and removing two positions and the trailing-zero offset leaves the remaining ones packed at the bottom, preserving popcount with the smallest increase. Two separate right shifts implement the shift by `trailingZeros(low)+2`, each count valid even when the sum reaches W.
- `grayDecode` computes prefix xors from the top with shifts 1, 2, 4, ..., < W (the doubling argument).

No performance comparison or speedup is claimed for these thin scalar operations, so no benchmark is required.

## Feature-to-test map

[`06-bit_operations_tester.py`](<../../96-Local Testing/06-Miscellaneous/06-bit_operations_tester.py>) includes the
actual header through its supporting C++ suite. Checks remain active under
NDEBUG. It requires Python's standard library and GCC only.

| Features | Independent checks |
|---|---|
| Counts, scans, powers, bit read/set/clear/flip | Bit-by-bit reference for 8/16/32/64/128-bit words; all zero/one/top/full/prefix masks; seeded words and exact Python integers. |
| Ceiling failure and mutation | Top-bit+1 through full-word cases, unchanged output on failure, input/output aliasing, zero→1 and constexpr checks. |
| Shifts and rotations | Reconstruct every output bit by position; zero/width shifts; all-int extreme rotation counts; Python rotation of binary strings. |
| Fixed-popcount successor | Independent set-position advancement; a descending exhaustive table of all 8-bit words (quick) and all 16-bit words (full/stress); zero/final mask preservation and high 128-bit transitions. |
| Submask stepping | Every 8-bit mask/submask pair; all subsets of four positions spanning each width's bottom/top halves; zero appears once; terminal mutation and constexpr checks. |
| Parity and Gray codes | Bit-by-bit references (adjacent-bit xor for the code, top-down running xor for the decode) on every exhaustive and random word of every width, both round trips; Python binary-string oracle for parity, code and decode on 15,764 (full) / 150,764 (stress) words; constexpr cases. |
| Supermask stepping | Every 8-bit (full, mask) pair with mask⊆full (6,561 pairs) against ascending brute enumeration, terminal preservation; random 8–128-bit (mask, full, sup) with the free-bit counter oracle (compressed free bits increase by exactly one), including the default full word and `sup == full`; constexpr case. |
| Type/value preconditions | Six compile constraint rejections; 16 checked-build assertion probes covering both bounds of every position/count API, an invalid submask, and supermask inputs that are not supersets, leave `full`, or have mask outside `full`. |

Quick uses 100 random words per width and exhaustive 8-bit words. Full uses
3000 random words per width and exhaustive 16-bit words. Stress raises random
coverage to 30000 per width; all modes include the same deterministic boundary
classes. The Python fixture generator independently checks count/width/scans,
low/floor/ceil, successor and rotations in 15,764 cases in full mode.

## Commands and results

P013 package run, 2026-10-07, GCC 16.2.1, GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan/UBSan `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all` with leak detection. Full: 9,243,635 C++ checks and 15,764 exact Python cases per configuration, 16 assertion probes and six compile rejections. Stress optimized: 38,727,635 C++ checks and 150,764 Python cases.

```sh
python3 '96-Local Testing/01-run.py' --mode quick --filter 06-Miscellaneous --no-integration                            # PASS (all 14 suites)
for s in 01-random 02-customhash 03-fastio 04-compression 05-binarysearch 06-bit_operations 07-permutation; do
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode full --seed 20260927                                # PASS x7, 3 configurations each
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode stress --seed 20260928 --configuration optimized   # PASS x7
done
python3 '96-Local Testing/06-Miscellaneous/03-fastio_benchmark.py'                                                       # PASS, record rewritten
python3 '96-Local Testing/02-integration.py'                                                                             # PASS (every header alone, Basic/All aggregates)
python3 '96-Local Testing/03-consistency.py'                                                                             # no errors
```

## Sources

Read during P013 on 2026-09-27/28; the implementation is independent and no
external source code was copied.

- [cp-algorithms, Submask Enumeration](https://cp-algorithms.com/algebra/all-submasks.html):
  descending-step proof, zero cycling, and the 2^k/3^n enumeration costs. The
  article's signed snippets were not adopted as signed APIs.
- [Sean Eron Anderson, Bit Twiddling Hacks, next bit permutation](https://graphics.stanford.edu/~seander/bithacks.html#NextBitPermutation):
  next fixed-popcount interpretation and scan/division alternatives, with the
  section's contribution dated 2009-11-28. The local carry-based derivation adds
  zero, final-word and narrow-width handling and uses separate safe shifts.
- Completeness sweep 2026-10-07 (`@researcher`): [cp-algorithms Gray code](https://cp-algorithms.com/algebra/gray-code.html), [Nyaan gray-code](https://nyaannyaan.github.io/library/math/gray-code.hpp) and [enumerate-set](https://nyaannyaan.github.io/library/set-function/enumerate-set.hpp) (superset step `(s + 1) | m`), [suisen bit_utils](https://suisen-cp.github.io/cp-library-cpp/library/util/bit_utils.hpp) (parity), [OI Wiki bit](https://oi-wiki.org/math/bit/), [cp-algorithms bit manipulation](https://cp-algorithms.com/algebra/bit-manipulation.html). Adopted parity, Gray encode/decode and the supermask step, generalized here to a universe `full`.
- Installed GCC 16 libstdc++ `<bit>`: count/scan zero handling, rotation counts,
  GNU 128-bit support and the explicitly invalid overflow domain of `bit_ceil`.
  This is inspection of the installed standard-library implementation, not an
  execution claim for every compiler at the library's GCC14+ floor.
- Independent review consulted the saved *Competitive Programmer's Handbook*,
  draft 2018-07-03, §10 pp97–99: scalar masks, lowest-bit operations and submask
  stepping. Its signed-shift/builtin-zero idioms informed boundary checks;
  neither the whole book nor unrelated chapters are claimed as reviewed here.

## Limits and handoffs

Omitted candidates: `countOnesUpTo(n)` (total popcount over `[0, n]`) needs more than `W` bits for full-range inputs and is digit-DP territory (`31-digit_dp.hpp`). `bitReverse` appeared in none of the swept catalogs. General combination generation and dynamic bitsets have separate owners.

Handoff to P015: `forEachSupermask` in `12-enumeration.hpp` can build on `BitOps::nextSupermask`. No online submission was made and no judge acceptance is claimed.

## History

- 2026-09-28: P013 first verification, full and stress suites passed.
- 2026-10-07: P013 re-audit, `parity`, `grayCode`, `grayDecode` and `nextSupermask` added, contracts moved out of code comments, brace style normalized; full, stress and integration passed; `@reviewer` (2026-10-08) found no defects.
