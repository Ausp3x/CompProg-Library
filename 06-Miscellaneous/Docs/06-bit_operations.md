# 06-bit_operations.hpp — Evidence

`06-bit_operations.hpp` (batch MI02, package P013; dependency P002, template) provides `BitOps<U>` (default `ulng`) for unsigned scalar words. Plain counts, scans, floors and rotations are the C++20 `<bit>` functions (Legacy Mapping). BitOps adds bit positions with -1 for zero, parity, rank and select, a checked ceiling, bit and bit-field manipulation, bit reversal, Gray and Morton codes, byte tests, mask and combination steps in both directions, the xor-submask lower bound and the xor-range block decomposition. Dynamic storage is Core Bitset; general enumerators are `12-enumeration.hpp`.

## Contracts

### BitOps

`U` is an unqualified unsigned type that `std::popcount` supports: unsigned char, short, int, long, long long and GNU unsigned 128-bit integers on this platform, so the aliases `uint`, `ulng`, `ulll`, `uint8_t` and `uint16_t` work. Signed types, `bool`, cv-qualified types and character code-unit types such as `char8_t` are rejected at compile time. Converting a signed value explicitly to an unsigned word gives its modulo-2^W representation; the API never shifts a signed value. `W` is the bit width, at most 128.

Every operation is `constexpr` with O(1) time and auxiliary memory at these fixed widths. `grayDecode`, `bitReverse`, `mortonEncode` and `mortonDecode` take O(log w) mask steps, at most 7; `countOnesUpTo` takes O(w) steps; `xorRangeBlocks` makes O(w) callback calls. `select` reads a 2 KB compile-time table (`bit_operations_detail::SELECT_TABLE`).

| Operation | Contract |
|---|---|
| `first(x)`, `last(x)` | Lowest/highest set-bit position, or -1 at x=0. |
| `lowBit(x)` | Lowest set-bit mask; zero at x=0. |
| `ceil(x,out)` | Smallest power of two ≥x, treating x=0 as 1. False when it exceeds the word, leaving out unchanged. Input and output may alias. |
| `lowMask(k)` | Lowest k bits set; require 0≤k≤W, including the full word. |
| `test(x,i)`, `set(x,i,on=true)`, `flip(x,i)` | Read/set/clear/flip bit i; require 0≤i<W. Mutation helpers return a word and do not change their input. |
| `shiftLeft(x,k)`, `shiftRight(x,k)` | Logical shifts; require 0≤k≤W. A shift of W returns zero. Left shift discards bits leaving the word. |
| `parity(x)` | Popcount modulo 2 as `bool`. |
| `countLeadingSignBits(x)` | The number of bits below the top bit that equal it, in `[0, W-1]` (GCC `__builtin_clrsb` on the word read as two's complement): `countl_one(x) - 1` when the top bit is set, `countl_zero(x) - 1` otherwise. |
| `countOnesUpTo(n)` | Total popcount of `0, 1, ..., n` as `ulll` (CSES 1146 Counting Bits), for words of at most 64 bits; a `requires` clause rejects a 128-bit word, whose total can exceed 128 bits. Bit `k` is set in exactly `floor(m/2^(k+1))·2^k + max(0, m mod 2^(k+1) - 2^k)` of the `m = n + 1` values, which the loop sums; at `n = 2^64 - 1` the total is `2^69`. |
| `grayCode(x)`, `grayDecode(x)` | Reflected binary Gray code `x ^ (x >> 1)` and its inverse (bit i of the decode is the xor of bits ≥ i); mutual inverses over the whole word. |
| `nextSupermask(sup,mask,full=~0)` | Require mask⊆full and mask⊆sup⊆full. Advance to the next larger set between mask and full, returning true; `sup == full` returns false unchanged. From `sup = mask` it visits all 2^popcount(full & ~mask) supersets in increasing order. |
| `prevSubmask(sub,mask)` | Require sub⊆mask. Advance to the next smaller submask, returning true; zero has no predecessor, returns false and stays zero. |
| `nextCombination(x)` | Advance to the smallest larger word with the same popcount. Zero and the last such word return false without mutation. |
| `prevCombination(x)` | Retreat to the largest smaller word with the same popcount. Zero, all-ones and `lowMask(popcount(x))` return false without mutation. |
| `nextSubmask(sub,mask)` | Require sub⊆mask. Advance to the next larger submask; `sub == mask` returns false unchanged. From `sub = 0` it visits all submasks in increasing order. |
| `prevSupermask(sup,mask,full=~0)` | Same requirements as `nextSupermask`. Retreat to the next smaller set between mask and full; `sup == mask` returns false unchanged. From `sup = full` it visits all supersets in decreasing order. |
| `rank(x,i)` | Number of set bits below position i; require 0≤i≤W. |
| `select(x,k)` | Position of the k-th set bit, counting from 0 at the lowest bit; -1 when k ≥ popcount(x). Require k≥0; any larger `int` is valid. `select(x, rank(x, i)) == i` for every set bit i. |
| `clearLowBit(x)`, `clearTrailingOnes(x)`, `setLowestZero(x)` | `x & (x-1)` (zero stays zero), `x & (x+1)` (all-ones gives zero), `x \| (x+1)` (all-ones stays all-ones), each modulo 2^W. |
| `mergeBits(a,b,mask)` | Bits of b where mask is set, bits of a elsewhere. |
| `signExtend(x,b)` | Read the low b bits as a two's-complement number and return its W-bit two's-complement form as `U` (bits ≥ b copy bit b-1); require 1≤b≤W. Convert to the signed type explicitly if needed. |
| `swapBitRanges(x,i,j,n)` | Exchange the n-bit fields starting at i and j; require 0≤n, 0≤i, 0≤j, i+n≤W, j+n≤W and disjoint fields (n=0 is a no-op, with i or j up to W). |
| `bitReverse(x)` | Bit i moves to bit W-1-i. |
| `mortonEncode(x,y)`, `mortonDecode(z)` | Interleave: bit i of x to bit 2i and bit i of y to bit 2i+1; require x, y < 2^(W/2). Decode returns the pair (even bits, odd bits) compacted, for every z; mutual inverses. |
| `hasZeroByte(x)`, `hasByteEqual(x,n)`, `hasByteLess(x,n)`, `hasByteGreater(x,n)`, `hasByteBetween(x,m,n)` | Whether some byte of x is 0, equals n (0≤n≤255), is below n (0≤n≤128), is above n (0≤n≤127), or lies strictly between m and n (0≤m≤127, 0≤n≤128). Exact answers; an 8-bit word is one byte. |
| `xorSubmaskLowerBound(b,s,lo,out)` | Smallest `y ≥ lo` of the form `b ^ t` with t⊆s, that is, agreeing with b outside s; false when none exists, leaving out unchanged. `out` may alias an input. |
| `xorRangeBlocks(l,r,x,f)` | Require l≤r. Calls `f(base, k)` for disjoint aligned blocks `[base, base + 2^k)` (low k bits of base zero, 0≤k<W) whose union is exactly `{y ^ x : l ≤ y < r}`; at most 2W calls, none for an empty range, in unspecified order. The range is half-open by the library convention, so `y = 2^W - 1` is not expressible; widen the word type to include it. |

The stepping operations cost O(1) per step, not per traversal; all submasks take Θ(2^popcount(mask)) time, zero included:

```cpp
ulng sub = mask;
do { /* process sub, including zero */ } while (BitOps<>::prevSubmask(sub, mask));
```

Position and count bounds and the subset conditions are asserted preconditions and remain caller obligations under NDEBUG. Overflow of a power-of-two ceiling and exhaustion of a mask sequence are valid results, reported by `false`.

Correctness: `first` branches on zero before `std::countr_zero`, and `last` uses the defined zero result of `std::bit_width` (0); `ceil` guards the only domain where `std::bit_ceil` would overflow, and width-equal branches in masks and shifts avoid shifting by the type's width. Narrow operands promote to `int`: the largest 16-bit shifted intermediate is 65535×2^15, which fits, every shift count stays below the promoted width, and explicit conversions restore 8/16-bit wrap after addition and complement.

- `prevSubmask`: for nonzero `sub`, subtracting one clears its lowest set bit and fills the positions below; intersecting with `mask` gives the largest smaller submask. Testing zero first prevents cycling back to `mask`.
- `nextSupermask`: the free bits `rest = full & ~mask` form a counter. Adding one to `sup | ~rest` carries exactly through the free positions in increasing order; masking with `rest` and restoring `mask` gives the next set. The only wrap is all free bits set, the `sup == full` case checked first.
- `nextCombination`: adding `low` (the lowest set bit) carries through the lowest run of ones, putting its top one in the next free higher position; wrapping to zero means no larger result. Otherwise `next ^ x` identifies the changed run, and removing two positions and the trailing-zero offset leaves the remaining ones packed at the bottom, preserving popcount with the smallest increase. Two separate right shifts implement the shift by `countr_zero(low)+2`, each count valid even when the sum reaches W.
- `prevCombination`: complement reverses the order of words and maps popcount c to W - c, so `~next(~x)` is the largest smaller word of popcount c; `nextCombination(~x)` fails exactly when no such word exists.
- `nextSubmask`: setting the bits outside mask makes `+1` carry only through mask positions, the binary successor of sub in mask's coordinates. `prevSupermask`: `(sup & rest) - 1` is the predecessor of the free-bit counter, restricted to `rest` and restored with mask; `sup == mask` (counter zero) is the only wrap.
- `grayDecode` computes prefix xors from the top with shifts 1, 2, 4, ..., < W (the doubling argument).
- `rank` masks below i (`lowMask` handles i = W). `clearLowBit`, `clearTrailingOnes`, `setLowestZero`: subtracting or adding one flips exactly the lowest set bit with the zeros below it, or the trailing ones with the zero above them.
- `select` runs a 64-bit kernel; a 128-bit word first chooses its half by the low half's popcount, and narrower words zero-extend. SWAR byte popcounts multiplied by `0x0101...` give in byte i the inclusive prefix count `s_i` of bytes 0..i (each at most 64, no carry). Since k < 64 after the range check, `((k·L | H) - s) & H` sets the high bit of exactly the bytes with `s_i ≤ k` (each byte computes `128 + k - s_i ≥ 64`, so no borrow crosses bytes). The counts are monotone, so these bytes form a prefix, and their number b, read by one more multiplication, is the byte holding the bit. Subtracting `s_{b-1}` (byte b of `s << 8`) gives the rank inside that byte, which the table resolves.
- `signExtend`: with `top = 2^(b-1)`, `(v ^ top) - top` maps v ≥ top to v - 2^b and leaves v < top unchanged, modulo 2^W.
- `swapBitRanges`: `t` is the xor of the two fields; xoring it into both positions swaps them, valid because the fields are disjoint. `shiftLeft`/`shiftRight` keep the n = 0, i = W case defined.
- `bitReverse` swaps adjacent blocks of size 1, 2, 4, ..., W/2 with `BLOCK_MASK[k] = (2^W - 1)/(2^(2^k) + 1)` (the low 2^k bits of every 2^(k+1)-bit block); composing the log2(W) block swaps maps bit i to W-1-i. Morton spread runs the same ladder downwards (`(v | v << s) & BLOCK_MASK`), and pack runs it upwards.
- Byte tests (from Bit Twiddling Hacks). `hasByteLess`: a byte with value v < n ≤ 128 and no incoming borrow becomes `256 + v - n ≥ 128` with its own high bit clear, so the lowest such byte is flagged. A borrow leaves only a byte below n or a byte receiving a borrow, so a chain starts at a true byte. A byte v ≥ n without borrow gives `v - n`, whose high bit set implies v ≥ 128, cleared by `~x`. Hence the any-test is exact though individual flags may not be. `hasZeroByte` and `hasByteEqual` are the n = 1 case after xor. `hasByteGreater`: v + 127 - n sets the high bit iff v > n for v < 128 (the `| x` covers v ≥ 128 > n); a carry out of a byte needs v ≥ 129 + n, itself a true byte. `hasByteBetween` combines the two tests on the low seven bits, with `~x` excluding bytes ≥ 128 (which exceed every n ≤ 128 bound); the exhaustive 8- and 16-bit tables check it.
- `xorSubmaskLowerBound`: the candidates are the words agreeing with b on `fixed = ~s`. If lo is one, it is the answer. Otherwise let h be the highest fixed bit where lo and b differ; every candidate copies b at h. If b has 1 there, keep lo above h, take b's 1 at h and the smallest completion below (fixed bits of b, free bits 0). If b has 0, every candidate equal to lo above h is smaller, so the answer must exceed lo at a free position p > h where lo has 0; the lowest such p gives the smallest answer (lo above p, 1 at p, minimal completion below); none means no candidate is ≥ lo.
- `xorRangeBlocks`: at step k both l and r are multiples of 2^k. If bit k of l is set and l < r, then l + 2^k ≤ r, so `[l, l + 2^k)` is an aligned block inside the range, and l becomes a multiple of 2^(k+1); symmetrically for r. When both are multiples of 2^(k+1) with l ≤ r, the rest is covered at larger k; at k = W both are zero, so the loop ends by k = W - 1. Xor with x maps an aligned block `[a, a + 2^k)` onto the aligned block whose base is `(a ^ x)` with the low k bits cleared, a bijection, so the images are disjoint and cover the image set. Each step emits at most two blocks.

### Legacy Mapping

Each replacement accepts every BitOps word type, including GNU `unsigned __int128` under `-std=gnu++20`, with the same zero and modulo-width results.

| Removed (prune, 2026-10-12) | Replacement |
|---|---|
| `count(x)`, `width(x)` | `std::popcount(x)`, `std::bit_width(x)` |
| `leadingZeros(x)`, `trailingZeros(x)` | `std::countl_zero(x)`, `std::countr_zero(x)` |
| `single(x)`, `floor(x)` | `std::has_single_bit(x)`, `std::bit_floor(x)` |
| `rotateLeft(x, k)`, `rotateRight(x, k)` | `std::rotl(x, k)`, `std::rotr(x, k)` |

## Feature-to-Test Map

| Features | Independent checks |
|---|---|
| `first`, `last`, `lowBit`, `lowMask`, bit read/set/clear/flip; the `<bit>` replacements of the removed counts, scans and floor | Bit-by-bit reference for 8/16/32/64/128-bit words; all zero/one/top/full/prefix masks; seeded words and exact Python integers. |
| Ceiling failure and mutation | Top-bit+1 through full-word cases, unchanged output on failure, input/output aliasing, zero→1 and constexpr checks. |
| Shifts; the `std::rotl`/`std::rotr` replacements of the removed rotations | Every output bit rebuilt by position; zero/width shifts; all-int extreme rotation counts; Python rotation of binary strings. |
| Fixed-popcount successor | Independent set-position advancement; a descending exhaustive table of all 8-bit words (quick) and all 16-bit words (full/stress); zero/final mask preservation and high 128-bit transitions. |
| Submask stepping | Every 8-bit mask/submask pair; all subsets of four positions spanning each width's bottom/top halves; zero appears once; terminal mutation and constexpr checks. |
| Parity and Gray codes | Bit-by-bit references (adjacent-bit xor for the code, top-down running xor for the decode) on every exhaustive and random word of every width, both round trips; Python binary-string oracle for parity, code and decode on 15,764 (full) / 150,764 (stress) words; constexpr cases. |
| Supermask stepping | Every 8-bit (full, mask) pair with mask⊆full (6,561 pairs) against ascending brute enumeration, terminal preservation; random 8–128-bit (mask, full, sup) with the free-bit counter oracle (compressed free bits increase by exactly one), including the default full word and `sup == full`; constexpr case. |
| `select`, `rank` | Running bit count over every exhaustive, mask and random word; `select(x, rank(x, i)) == i` at every set bit; `rank` at every i in [0, W]; -1 at k = popcount, popcount+1 and `INT_MAX`; 128-bit words crossing the 64-bit split; Python exact fixture; constexpr case. |
| `bitReverse`, `clearLowBit`, `clearTrailingOnes`, `setLowestZero`, `mergeBits` | Bit-by-bit rebuild on the same word classes with zero and all-ones; `bitReverse` as its own inverse and against Python binary-string reversal; per-bit choice for `mergeBits` on sparse and dense words. |
| `signExtend`, `swapBitRanges` | Every b in [1, W] on every checked word, Python binary-string oracle; per-bit source mapping for every valid (i, j, n) on all 8-bit words, eight boundary triples per word (n = 0 with i or j = W, swapped halves) and random disjoint fields. |
| `mortonEncode`, `mortonDecode` | Bit placement both ways, inverse checks, all-ones halves, random halves, Python exact fixture. |
| Byte tests | Bytes read separately. All 8-bit words with every n and m (all modes); all 16-bit words with every n and 8 values of m each (full, stress); random wider words built from bytes biased to 0, 1, 126–129, 254, 255 and the parameter ±1, with every byte failing the test and half the time one passing. |
| `nextSubmask`, `prevSupermask`, `prevCombination` | Exhaustive 8-bit traversals against ascending or descending brute lists, the terminal step returning false unchanged, the default `full`; `prevCombination` against tables of all 8-bit (and in full all 16-bit) words and a set-position reference; random wide words with a free-bit counter (exactly ±1); round trips with `nextCombination`, `nextSupermask` and `prevSubmask`. |
| `xorSubmaskLowerBound` | Upward-scan brute over 8-bit (b, s, lo): 16 values of b in quick, all 2^24 triples in full and stress, output aliased with lo and with b, output unchanged on false; wide words against a separately validated lowest-candidate search (s = 0, all ones, lo = 0, all ones, next to a member) and 8-bit problems embedded at random positions of 128-bit words. |
| `xorRangeBlocks` | Every 8-bit l ≤ r pair with a stamp array (disjointness, membership, total, alignment, k < 8, at most 16 calls) for 4 values of x in quick, 40 in full, all 256 in stress; wide ranges (short, ending at the top value, empty, 0 to top, random large) mapped to aligned disjoint intervals summing to r − l, enumerated exactly up to length 1024; Python set check on every fixture; constexpr case. |
| `countLeadingSignBits`, `countOnesUpTo` | Sign-bit run counted bit by bit from the top on every exhaustive, mask and random word of every width; `countOnesUpTo` against the top-bit recursion f(2^k + r) = k·2^(k-1) + r + 1 + f(r) in `ulll` on every checked word of width at most 64, against a running popcount sum for every n below 2^8 (8-bit words) and 2^20 (16-, 32- and 64-bit words), and against the closed form k·2^(k-1) at every n = 2^k - 1 including 2^64 - 1; the compile rejection of `BitOps<ulll>::countOnesUpTo`. |
| Type/value preconditions | Seven compile constraint rejections (the six type constraints and the 128-bit `countOnesUpTo`); 47 checked-build assertion probes: both bounds of every position/count API, an invalid submask, supermask inputs that are not supersets, leave `full` or have mask outside `full`, position W+1 for `test`, `set` and `flip`, `select` k < 0, `rank` i = -1 and W+1, `signExtend` b = 0 and W+1, `swapBitRanges` overlap, n < 0, i < 0, i+n > W, j+n > W, `mortonEncode` oversized x, oversized y and equal oversized halves, each byte test's parameters below and above range, `nextSubmask` and `prevSupermask` invalid sets, `xorRangeBlocks` l > r. |

Modes: quick uses 100 random words per width and exhaustive 8-bit words; full 3000 random words per width and exhaustive 16-bit words; stress 30000 random words per width. Every mode includes the same deterministic boundary classes. The Python fixture generator independently checks positions, `lowBit`, `ceil`, successor, parity and Gray codes, and the `<bit>` replacements of the removed members, in 15,764 cases in full mode.

## Commands and Results

P013 MI02 `/finish` runs on 2026-11-17: Intel Core i9-11900H, Linux x86-64, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14; optimized, checked and ASan-UBSan builds from [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>), run beside other suites and subagents.

| Date | Command | Result |
|---|---|---|
| 2026-11-17 | `python3 '96-Local Testing/06-Miscellaneous/06-bit_operations_tester.py' --mode full --seed 1` | PASS, 3 configurations, 158,712,797 C++ checks and 15,764 exact Python cases per configuration, 47 assertion probes, seven compile rejections; run optimized 3.4 s, checked 6.7 s, ASan-UBSan 16.5 s; `MEMORY peak=1590MB` (pre-flight quick 1081 MB, cold compiler cache) |
| 2026-11-17 | same with `CXX=g++-14` | PASS, same counts, `MEMORY peak=1584MB` (pre-flight quick 287 MB) |
| 2026-11-17 | `python3 '96-Local Testing/06-Miscellaneous/06-bit_operations_tester.py' --mode stress --seed 2` | PASS, 217,740,703 C++ checks and 150,764 Python cases per configuration, 47 probes; ASan-UBSan 75.5 s; `MEMORY peak=1179MB` |
| 2026-11-17 | `python3 '96-Local Testing/05-extraction.py' ops --filter 06-Miscellaneous/06-bit_operations.hpp` | PASS, 39 operations in 39 extractions on g++ and g++-14, each instantiating its default form |
| 2026-11-17 | `python3 '96-Local Testing/05-extraction.py' testers --filter 06-bit_operations_tester` | PASS, quick run on the extracted build |
| 2026-11-17 | `python3 '96-Local Testing/02-integration.py' --sanitizers` | PASS, 192 standalone and aggregate headers, scalar and available AVX2 multi-TU builds, workspace, sanitizer self-tests, rerun on the final headers; `MEMORY peak=2703MB` |
| 2026-11-17 | `python3 '96-Local Testing/04-mutation.py' run 06-Miscellaneous/06-bit_operations.hpp` (changed lines) | Record under Mutation Testing |

## Benchmarks

No speedup is claimed for these thin scalar operations except the `select` kernel choice. Measured 2026-11-16 on an Intel Core i9-11900H, Linux x86-64, GCC 16.2.1 `-O2`: 2^20 words `rng() & rng()` (density 1/4) from `std::mt19937_64` seed 1, each queried for a k drawn below its popcount, 20 passes, results summed into a printed checksum, one run each, total time over queries. The table version took 5.5 ns per query, the same SWAR step with a clear-k-lowest-bits loop instead of the table 17 ns, and a binary search over half-word popcounts 57 ns (34 ns with `-march=native`). The table version is kept; the scratch driver was not.

## Mutation Testing

Record: mewt 4.0.0 on blob `44720789a5dfc948e8beb0e6d372f510c62945d5` (2026-11-17, seed 1, 14 lines changed since blob `3964361607c4`): 110 mutants, 105 caught (0 by timeout), 5 survived, 0 untested.

The record covers `countLeadingSignBits`, `countOnesUpTo` and the `l`/`r` rename in `xorRangeBlocks`, sampling 110 of 1169 generated mutants; stage seconds were quick 734, full optimized 66, full checked and ASan-UBSan 256. Earlier campaigns: a whole-file campaign on 2026-11-16 (500 of 1128 mutants, 18 survivors; four closed by new probes, three by removing redundant assertions), a diff-scoped one the same day on the trimmed assertions (39 mutants, 4 survivors), and one on a comment-only change (0 mutants). Every open survivor is equivalent:

| Mutant | Line | Why it survives |
|---|---|---|
| `COS 826` | 72 | `k != W` for `k < W` in the `countOnesUpTo` loop: k starts at 0 and rises by one. |
| `COS 827` | 72 | `k <= W`: the extra term k = W has `half = 2^W >= m`, so `m / (2 * half) = 0` and `rest = m` is not above `half`; it adds 0. |
| `AOS 394` | 74 | `rest % half` for `rest - half`: the branch runs only for `half < rest < 2 * half`, where both are equal. |
| `COS 834` | 74 | `rest >= half`: at `rest == half` the added term `rest - half` is 0. |
| `COS 1096` | 176 | `l != r` for `l < r` in `xorRangeBlocks`: l ≤ r holds at every step (Contracts). |
| `COS 997`, `COS 1012` | 142, 147 | From the 2026-11-16 assertion-trim record: `(sup & mask) >= mask` for `==` in the `nextSupermask`/`prevSupermask` assertions; a submask of mask is at least mask only when equal. |
| `COS 1000`, `COS 1015` | 142, 147 | Same record: `(sup & ~full) <= 0` for `== 0`, the same test on an unsigned word. |
| `COS 724`, `COS 729` | 9, 10 | From the 2026-11-16 whole-file record: `v <= 256`, `i <= 8` in the table builder; bit i < 8 of 256 and bit 8 of a byte are zero, so the extra iterations write nothing. |
| `COS 748` | 45 | Whole-file record: `k + 1 != LOG_W` in `pack`; k starts below the bound (LOG_W ≥ 3) and rises by one. |
| `COS 806`, `COS 851` | 83, 96 | Whole-file record: `k >= W` for `k == W` in `lowMask` and `shiftLeft`; k ≤ W is asserted, and past the violated precondition both still end in O(1). |
| `BOS 580` | 110 | Whole-file record: `\|` to `^` in `bitReverse`; the two terms occupy disjoint bit sets (`m` and its shift). |
| `COS 995` | 131 | Whole-file record: `!= 0` to `> 0` on an unsigned word in `hasByteBetween`. |
| `COS 1011` | 139 | Whole-file record: `sub >= mask` for `sub == mask` in `nextSubmask`; a submask is at most its mask, with equality only for the mask. |
| `AOS 478` | 166 | Whole-file record: `lowMask(h * 1)` in the first term; bit h of lo is 0 in that branch (b has 1 and they differ there). |
| `AOS 486` | 167 | Whole-file record: `lowMask(h * 1)`; bit h is fixed, so it is never in `s` and `up` is unchanged. |

## Sources

The implementation is independent; no external code was copied.

- Read during P013 on 2026-09-27/28: [cp-algorithms, Submask Enumeration](https://cp-algorithms.com/algebra/all-submasks.html) for the descending-step proof, zero cycling and the 2^k/3^n enumeration costs (its signed snippets were not adopted as signed APIs); [Sean Eron Anderson, Bit Twiddling Hacks, next bit permutation](https://graphics.stanford.edu/~seander/bithacks.html#NextBitPermutation) for the fixed-popcount successor and its scan/division alternatives (section contribution dated 2009-11-28). The local carry-based derivation adds zero, final-word and narrow-width handling and uses separate safe shifts.
- Completeness sweep 2026-10-07 (`@researcher`): [cp-algorithms Gray code](https://cp-algorithms.com/algebra/gray-code.html), [Nyaan gray-code](https://nyaannyaan.github.io/library/math/gray-code.hpp) and [enumerate-set](https://nyaannyaan.github.io/library/set-function/enumerate-set.hpp) (superset step `(s + 1) | m`), [suisen bit_utils](https://suisen-cp.github.io/cp-library-cpp/library/util/bit_utils.hpp) (parity), [OI Wiki bit](https://oi-wiki.org/math/bit/), [cp-algorithms bit manipulation](https://cp-algorithms.com/algebra/bit-manipulation.html). Parity, Gray encode/decode and the supermask step were adopted, the last generalized to a universe `full`.
- Installed GCC 16 libstdc++ `<bit>`: zero handling of counts and scans, rotation counts, GNU 128-bit support and the invalid overflow domain of `bit_ceil`. This is inspection of the installed implementation, not an execution claim for every compiler at the GCC 14 floor.
- An independent review consulted the saved *Competitive Programmer's Handbook*, draft 2018-07-03, §10 pp97–99 (scalar masks, lowest-bit operations, submask stepping); its signed-shift and builtin-zero idioms informed boundary checks. No other chapter is claimed as reviewed.
- P013 MI02 sweep 2026-11-16 (pages in [00-sources.md](00-sources.md), P013 MI02 sweep): maspypy [`xor_submask_lower_bound`](https://raw.githubusercontent.com/maspypy/library/main/setfunc/xor_submask_lower_bound.hpp) (`xorSubmaskLowerBound`), [`my_template`](https://raw.githubusercontent.com/maspypy/library/main/my_template.hpp) (`bitReverse`) and [`dyadic_intervals`](https://maspypy.github.io/library/other/dyadic_intervals.hpp) (`xorRangeBlocks` at x = 0); [cp-algorithms bit manipulation](https://cp-algorithms.com/algebra/bit-manipulation.html) (`clearLowBit`, `clearTrailingOnes`, `setLowestZero`); a [mirror of Bit Twiddling Hacks](https://raw.githubusercontent.com/frederik-hoeft/bit-twiddling-hacks/main/README.md) (the original's certificate was rejected) for `rank`, Morton interleave masks, `signExtend`, `mergeBits`, `swapBitRanges` and the five byte tests. `nextSubmask`, `prevSupermask`, `prevCombination` and `select`'s byte step are derived here (Contracts).
- P013 MI02 `/finish` sweep 2026-11-17 (pages in [00-sources.md](00-sources.md), P013 MI02 `/finish` sweep): [cp-algorithms bit manipulation](https://cp-algorithms.com/algebra/bit-manipulation.html) and the [CSES 1146](https://cses.fi/problemset/task/1146) statement for `countOnesUpTo`; [OI Wiki 位运算](https://oi-wiki.org/math/bit/) (`__builtin_clrsb`) for `countLeadingSignBits`.

## Limits and Handoffs

- Not adopted (user decisions 2026-11-17 in `omissions.md`): BMI2 `pdep`/`pext` and a `pdep`-based select are ISA-specific, outside the Contest profile, and no catalog names a portable compress/expand of bits. General combination generation, Gray sequences (`12-enumeration.hpp` `forEachGrayMask`), rank/select bit vectors (Data Structures `SuccinctBitVector`) and dynamic bitsets have separate owners; signed bit tricks are covered by `std::<bit>` on the unsigned word.
- Name clash: maspypy `lowbit` is a position (here `first`), while `lowBit` here is a mask.

## Aliases

Search aliases (P263, grab --find): 格雷码, 位操作, 二进制集合操作, 位运算, 位运算技巧, 比特运算, 枚举子集, 枚举超集, 奇偶校验, 下一个组合, select64, kth set bit, bit_reverse, 位反转, morton code, z-order, 莫顿码, sign extend, haszero, xor_range, xor_of_interval, dyadic_intervals, xor_submask_lower_bound, topbit, lowbit, msb, lsb, kth_bit, full_mask, clrsb, leading redundant sign bits, Counting Bits, popcount prefix sum.

## History

- 2026-09-28: P013 first verification; full and stress passed.
- 2026-10-07: P013 re-audit added `parity`, `grayCode`, `grayDecode` and `nextSupermask` and moved contracts out of code comments; full, stress and integration passed; `@reviewer` (2026-10-08) found no defects.
- 2026-10-12: prune removed `count`, `width`, `leadingZeros`, `trailingZeros`, `single`, `floor`, `rotateLeft`, `rotateRight` (cases moved to the `<bit>` replacements, `12-enumeration.hpp` migrated); full passed on g++ and g++-14.
- 2026-11-07: P263 follow-up added Chinese search aliases (OI Wiki page titles and common names, Simplified plus Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 (SUP25) moved builds to the shared runner `_00_contest_runner.py` with its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; library-wide runs passed.
- 2026-11-16: P013 MI02 added `select`, `rank`, `xorRangeBlocks`, `xorSubmaskLowerBound`, `bitReverse`, `clearLowBit`, `clearTrailingOnes`, `setLowestZero`, `mergeBits`, `signExtend`, `swapBitRanges`, `mortonEncode`/`mortonDecode`, five byte tests, `nextSubmask`, `prevSupermask` and `prevCombination` (user decisions 2026-11-16) and trimmed redundant assertions after mutation; full and stress passed on g++ and g++-14.
- 2026-11-17: P013 MI02 `/finish` added `countOnesUpTo` and `countLeadingSignBits` (user decisions 2026-11-17) and rewrote the omission reasons; full and stress passed on g++ and g++-14; mutation campaign on the changed lines left 5 equivalent survivors.
