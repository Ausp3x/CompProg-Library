# Scalar word operations — MI02 / P013

`06-bit_operations.hpp` provides `BitOps<U>` (default `ulng`) for unsigned scalar
words. It delegates standard operations to C++20 `<bit>` and adds safe boundary
contracts, individual-bit manipulation and two constant-time mask steps. It
does not implement dynamic storage or general enumerators; those remain in Core
Bitset and MI15 enumeration respectively.

## Domains and API

`U` must be an unqualified unsigned type supported by `std::popcount`: unsigned
char/short/int/long/long long and GNU unsigned 128-bit integers on this platform.
The shared aliases `uint`, `ulng`, `ulll` and `uint8_t`/`uint16_t` work. Signed
types, `bool`, cv-qualified types and character code-unit types such as `char8_t`
are rejected at compile time. Explicitly converting a signed value to a chosen
unsigned word gives its modulo-2^W representation; the API never performs an
implicit signed shift. `W` is the type's bit width, at most 128.

Every operation is `constexpr`, O(1) time and O(1) auxiliary memory for these
fixed scalar widths. The compiler chooses ordinary scalar instructions; there
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

## Correctness and implementation choices

The standard count/scan/rotation operations have defined zero and modulo-width
semantics. `ceil` guards the only domain where `std::bit_ceil` would overflow.
The width-equal branches in masks/shifts avoid evaluating a shift by the type's
width. Narrow unsigned operands can promote to `int`; the largest offered
16-bit shifted intermediate is 65535×2^15, which still fits `int`. Explicit
conversions restore word wrap after addition and complement, particularly for
8/16-bit words. Unsigned 32/64/128-bit arithmetic wraps by definition.

For a nonzero submask, subtracting one clears its lowest set bit and fills the
positions below it. Intersecting with `mask` gives exactly the largest admissible
smaller submask. Testing zero before subtraction prevents cycling back to the
original mask.

For a fixed-popcount successor, let `low` be the lowest set bit. Adding it to
`x` carries through the lowest consecutive run of ones, putting that run's top
one in the next free higher position. If the word wraps to zero there is no
larger result. Otherwise `next ^ x` identifies the changed run; removing two
positions and the trailing-zero offset leaves exactly the remaining one bits
packed at the bottom. Combining them with `next` preserves popcount and makes
the smallest possible increase. Two separate right shifts implement the
effective shift by `trailingZeros(low)+2`; their individual counts are valid
even when their sum reaches or exceeds W. No division or scan of a zero word
is needed. These arguments, plus the standard-library contracts, justify the
algorithms beyond the finite tests.

No performance comparison or universal speedup is claimed for these thin scalar
operations. Standard compiler implementations and constant-time stepping avoid
an unnecessary local bit-count/scan engine. General combination generation and
dynamic bitsets have separate owners.

## Inspected references

Read during P013 on 2026-09-27/28; the implementation is independent and no
external source code was copied.

- [cp-algorithms, Submask Enumeration](https://cp-algorithms.com/algebra/all-submasks.html):
  descending-step proof, zero cycling, and the 2^k/3^n enumeration costs. The
  article's signed snippets were not adopted as signed APIs.
- [Sean Eron Anderson, Bit Twiddling Hacks, next bit permutation](https://graphics.stanford.edu/~seander/bithacks.html#NextBitPermutation):
  next fixed-popcount interpretation and scan/division alternatives, with the
  section's contribution dated 2009-11-28. The local carry-based derivation adds
  zero, final-word and narrow-width handling and uses separate safe shifts.
- Installed GCC 16 libstdc++ `<bit>`: count/scan zero handling, rotation counts,
  GNU 128-bit support and the explicitly invalid overflow domain of `bit_ceil`.
  This is inspection of the installed standard-library implementation, not an
  execution claim for every compiler at the library's GCC14+ floor.
- Independent review consulted the saved *Competitive Programmer's Handbook*,
  draft 2018-07-03, §10 pp97–99: scalar masks, lowest-bit operations and submask
  stepping. Its signed-shift/builtin-zero idioms informed boundary checks;
  neither the whole book nor unrelated chapters are claimed as reviewed here.

## Feature-to-test map

`96-Local Testing/06-Miscellaneous/06-bit_operations_tester.py` includes the
actual header through its supporting C++ suite. Checks remain active under
NDEBUG. It requires Python's standard library and GCC only.

| Features | Independent checks |
|---|---|
| Counts, scans, powers, bit read/set/clear/flip | Bit-by-bit reference for 8/16/32/64/128-bit words; all zero/one/top/full/prefix masks; seeded words and exact Python integers. |
| Ceiling failure and mutation | Top-bit+1 through full-word cases, unchanged output on failure, input/output aliasing, zero→1 and constexpr checks. |
| Shifts and rotations | Reconstruct every output bit by position; zero/width shifts; all-int extreme rotation counts; Python rotation of binary strings. |
| Fixed-popcount successor | Independent set-position advancement; a descending exhaustive table of all 8-bit words (quick) and all 16-bit words (full/stress); zero/final mask preservation and high 128-bit transitions. |
| Submask stepping | Every 8-bit mask/submask pair; all subsets of four positions spanning each width's bottom/top halves; zero appears once; terminal mutation and constexpr checks. |
| Type/value preconditions | Six compile constraint rejections; 13 checked-build assertion probes covering both bounds of every position/count API and an invalid subset. |

Quick uses 100 random words per width and exhaustive 8-bit words. Full uses
3000 random words per width and exhaustive 16-bit words. Stress raises random
coverage to 30000 per width; all modes include the same deterministic boundary
classes. The Python fixture generator independently checks count/width/scans,
low/floor/ceil, successor and rotations in 15,764 cases in full mode.

Full verification on GCC 16.2.1 / GNU++20, seed `20260927`, passed optimized
`-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and ASan/UBSan with leak
checking: **8,694,112 C++ checks and 15,764 exact Python cases per configuration**,
plus all 13 assertion probes and six compile rejections. The initial sanitizer
attempt hit LeakSanitizer's sandbox ptrace limitation; the approved final run
outside the sandbox passed all three configurations with the added high-bit
submask and output-alias cases.

```sh
python3 '96-Local Testing/06-Miscellaneous/06-bit_operations_tester.py' --mode full --seed 20260927
```

Stress mode is supplied; the full result does not claim a stress run. Package-wide standalone/aggregate/multiple-TU and consistency results are
recorded in [P013 evidence](95-p013.md).
