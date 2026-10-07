# 18-bitset.hpp — evidence

`18-bitset.hpp` provides `Bitset`, a runtime-length array of bits backed by contiguous 64-bit words. C15 follows [C01](01-template.md) in P002. The implementation is independent of BitMatrix and has no runtime CPU dispatch. All arithmetic and indexing use the GNU C++20 x86-64 contract (Linux and Windows/MinGW, see [01-template.md](01-template.md#platform-and-includes)).

## Contracts

### Bitset

- `Bitset(n, value)` initializes exactly `n` bits. `Bitset(string_view)` reads a binary string most significant bit first; the empty string is an empty set. `fromWords(n, words)` requires exactly `n / 64 + (n % 64 != 0)` words in least significant word order and discards high padding bits. `toString()` and `blocks()` export these respective layouts; the `debugString` ADL hook exposes the binary string to C01 debug formatting.
- `size`, `empty`, `clear`, `resize`, `pushBack`, `popBack`, copy/move and `swap` manage storage. Growth preserves existing bits and fills new bits with the supplied value. Shrink discards removed bits permanently. Move leaves the source empty; self-move preserves it. Allocation failure propagates the standard allocation exception; lengths must be feasible for `vector<uint64_t>` on the host.
- `test(i)` and const indexing return a bit. Mutable indexing returns an assignable, copyable Boolean proxy with `flip`; copies alias the same bit and proxy-to-proxy assignment copies the bit value. `set(i, value = true)`, `reset(i)`, `flip(i)` return the set itself. No-argument `set/reset/flip` act on the whole set. Indexing requires `i < size`.
- `setRange(l,r,value)`, `resetRange`, `flipRange`, `rangeMask(n,l,r)` and `count(l,r)` use half-open ranges, requiring `0 <= l <= r <= n`. Empty ranges are valid. Count, `any`, `none`, and `all` consider exactly the logical bits; `all()` of the empty set is true.
- `findFirst`, `findLast`, `findNext(p)`, `findPrev(p)` return `size()` when no set bit exists. Next and previous are strict; `p <= size()` is required. `findNext(size())` is absent and `findPrev(size())` finds the last set bit. Mutation between repeated scans has ordinary current-value semantics.
- `&`, `|`, `^`, `-` (set difference), their compound assignments, `~`, `intersects`, `isSubsetOf`, `countAnd`, `countOr`, and `countXor` operate on packed words. Binary operations require equal lengths. Equality is defined even for unequal lengths and returns false in that case; C++20 supplies `!=`.
- `<<`, `>>` and compound shifts keep the length fixed, shift toward higher/lower bit indices, and zero vacated positions. Every `size_t` shift amount is supported, including `SIZE_MAX`; shifts at least the length produce zero. Mutating `rotateLeft/rotateRight` reduce modulo a nonzero length; empty rotations do nothing.
- `combine<Op>(b)` and `combineShift<Op>(b,k,left)` provide assignment, AND, OR, XOR and AND-NOT (`Op::Assign/And/Or/Xor/AndNot`) without a temporary shifted allocation. The latter is exactly `old_this OP shifted(old_b)` with equal lengths, including self-aliasing. `left` defaults to true. For example, subset-sum uses `bits.combineShift<Bitset::Op::Or>(bits, weight)`.
- `combineRange<Op>(l, r, b, p = 0)` applies `this[l + j] OP= b[p + j]` for `j` in `[0, r - l)`; it requires `l <= r <= size()` and `p + (r - l) <= b.size()`, lengths may differ, bits outside `[l, r)` are untouched, and `b` may alias the receiver (its source range `[p, p + r - l)` is then copied first through `slice`, so original-value semantics hold at O(1 + (r - l) / 64) cost). `xorSuffix(i, b)` of other libraries is `combineRange<Op::Xor>(i, n, b, i)`. `slice(l, r)` returns a new `(r - l)`-bit set holding bits `[l, r)`.

The fields and kernel helpers (`apply`, `applyVector`, `popcounts`, `countWords`, `shiftWords`, `range`, `countWith`) remain public to follow the contest-library convention; they are representation details rather than extra supported operations. External code must not mutate `n` or `a`, call unlisted kernels, or retain a mutable raw alias to storage. All API operations maintain `a.size() == ceil(n / 64)` and zero unused high bits. Bit proxies and block spans invalidate on resize, clear, push/pop, assignment, move and swap. Other bit mutations preserve them. There are no owning iterators or hidden caches, and concurrent mutation is outside the contract.

Runtime length `n`, bit 0 least significant, ranges half-open `[l, r)`. Boolean operations, `isSubsetOf` and `intersects` require equal sizes; equality does not. Shifts by at least `size()` yield zero; rotations reduce modulo `size()` and leave an empty set unchanged. Scans return `size()` on absence; `findNext`/`findPrev` are strictly after/before `p <= size()`. Public `n` and `a` are representation, not mutation APIs: `a.size() == ceil(n / 64)` with zero padding. Proxies and block spans invalidate on resize, clear, push/pop, move, assignment and swap; other mutations preserve them, and scans keep no persistent iterators. Not thread-safe. Every mutating operation supports self-aliasing; moved-from objects are empty. Resize costs worst-case O(old words + new words) on growth and O(1) on shrink; `pushBack` is amortized O(1); `popBack`, `clear`, `swap`, `size`, `empty` and `blocks` are O(1). Rotation, copies and nonmutating operators use O(ceil(n / 64)) extra result or workspace.

### shiftWords

`dst[i] OP= word i of x shifted by s < 64` for `i` in `[0, words)`. Left reads `x[i]`, `x[i - 1]` descending; right reads `x[i]`, `x[i + 1]` ascending. The outer neighbour (`x[-1]` or `x[words]`) is read only when `s != 0`. Each group loads its sources before storing, so `dst` may overlap `x` in the direction of travel.

### combineShift

`this OP= (b shifted k)` with equal sizes; `b` may alias `this` with original-value semantics, still in O(1) workspace.

### combineRange

`this[l, r) OP= b[p, p + (r - l))`; sizes may differ, and when `b` aliases `this` its source range is copied first, costing O(1 + (r - l) / 64) workspace.

### Complexity and correctness argument

Let `W = ceil(n / 64)`. Bulk operations and scans take worst-case `O(W)` time; scans and Boolean predicates stop early when possible. Single-bit access, size, clear, swap and span access take constant time. Range work takes `O(1 + (r-l)/64)`. Growing resize takes worst-case `O(old W + new W)` if allocation moves existing words; pushBack is amortized constant time. Shrink/pop are constant time. Text import/export take `O(n)` time. Stored memory is `O(W)`; text export uses `O(n)` returned memory, copying/nonmutating operators and rotation use `O(W)` result/workspace, and fused mutations use constant workspace.

Bit position `i` is word `i/64`, offset `i%64`. Splitting each shift into whole-word displacement `d` and a residual `s` in `[0,63]` gives the two contributing words. The shared kernel `shiftWords<Op>(dst, x, words, s, left)` applies `dst[i] OP= x[i] << s | x[i-1] >> (64-s)` (left, descending) or `dst[i] OP= x[i] >> s | x[i+1] << (64-s)` (right, ascending) and reads the outer neighbour `x[-1]`/`x[words]` only when `s != 0`; a residual-zero branch avoids scalar shifts by 64. `combineShift` passes the interior words so that both neighbours exist and finishes the single boundary word (`a[d]` for left, `a[size-d-1]` for right) after the kernel, which preserves the descending/ascending order that self-aliasing needs. Each SIMD group loads all contributing source words before writing its four destinations, so the same proof applies to self-aliasing. `combineRange` masks its two edge words with guarded source reads (missing source words read as zero, which only affects masked-out bits because every in-range destination bit maps into `[p, p + r - l)`) and hands the fully-covered interior words to `shiftWords`; for interior words both source neighbours are in bounds because the lowest interior bit maps to a source position `>= p >= 0` and the highest to a position `< p + (r - l) <= b.n`. Aliased calls copy the source range first, so no read sees a modified word. Vacated words are cleared for assignment/AND, and skipped for OR/XOR/AND-NOT. Range count masks its two endpoint words and passes the complete interior words to the same scalar/AVX2 count kernel. All paths mask the last word after shifts, rotations and length-changing operations. Whole-word Boolean operations preserve zero padding. These invariants justify scans, count, equality and subset checks without inspecting storage past its end.

Word counts use division plus a remainder test, without overflowing `n + 63`. Shift amounts are compared against `n` before displacement arithmetic. Range masks handle 0 and 64 explicitly. Scan sentinels never collide with valid indices. The AVX2 popcount uses duplicated nibble tables, byte counts at most eight, and 64-bit SAD reductions, so there is no byte accumulator overflow.

### Optimization scope

The baseline uses ordinary word arithmetic and `std::popcount`, with no required ISA flags. `__AVX2__` selects unaligned four-word directional fused shift and nibble-table population-count kernels, including fused Boolean counts; scalar tails handle every alignment and length. These kernels start at 16 words of kernel work (whole counts from 961 bits; for fused shifts the count excludes the boundary word, so from 1,025 bits with `k < 64`). Plain Boolean loops rely on compiler vectorization: the initial explicit AVX2 loop was measurably slower and was removed. Benchmark evidence below records the final selection. FMA and floating-point flags are irrelevant to exact bit operations. BMI-specific scan instructions can be emitted by the compiler when the caller enables them; the implementation requires neither BMI nor POPCNT.

The scope review added word/string conversion, subset/intersection queries and counts, last-bit scans, proxy assignment, and explicit move semantics to the starting inventory. Fixed-size `std::bitset` remains the compact alternative. Growable word append, formatted stream extraction, hashing/ordering, unset-bit scans and proxy compound assignment are convenience extensions rather than promised C15 features; they can be composed from the supported API. Succinct rank/select, compressed/Roaring bitmaps and sparse set representations belong to Data Structures. AVX512 VPOPCNTDQ and Harley–Seal carry-save population count are alternative backend research, not implemented or claimed verified here; this package does not promise every ISA or every published popcount backend.

## Feature-to-test map

The independent byte-per-bit oracle lives in [18-bitset_tester.cpp](<../../96-Local Testing/01-Core/18-bitset_tester.cpp>), with the runnable [Python entry](<../../96-Local Testing/01-Core/18-bitset_tester.py>). The entries accept `--mode quick|full|stress`, `--seed`, and the shared runner’s `CP_TEST_MODE/CP_TEST_SEED`. They run from an unrelated working directory, retain checks under `-DNDEBUG`, fail on subprocess/timeout errors, and report unavailable ISA execution as SKIP.

| Public feature | Independent coverage |
|---|---|
| Construction, word/string import/export, debug hook, size/empty | Per-bit reconstruction, MSB-first strings, dirty imported padding, empty and exact/partial words, LOCAL debug across translation units. |
| Access/proxies, set/reset/flip, ranges and range masks | Exhaustive small patterns/ranges, first/last bits, all word boundaries, stable proxy assignment/flip and mutation histories. |
| Count/any/none/all, scans | Byte sums and independently scanned positions, empty/zero/all-one/sparse/random patterns, strict scan endpoints and sentinel. |
| Boolean operators and compounds, complements, equality, subset/intersection and fused counts | Exhaustive input pairs through length six, byte Boolean oracles, all self-alias cases, unequal-size equality and checked preconditions. |
| Shifts/rotations and all five fused operations | Exhaustive tiny inputs, both directions, self/distinct source, 0/63/64/65/length/SIZE_MAX amounts, word/AVX2/dispatch neighbors and nonzero tails. |
| Resize, clear/reuse, push/pop, copy/move/swap | Shrink/regrow with both fill values, growth through multiple words, unchanged-size resize, source emptiness after move, self-copy/move/swap and randomized operation histories. |
| Preconditions | 34 separate assertion-death cases: access, malformed ranges/string/word count, scan position, empty pop, and every equal-size operation family, `combineRange` order/end/source/offset and `slice` order/end. |
| `slice` | Exhaustive all `(l, r)` for every pattern through length 4 (5 in stress); word-boundary `(l, r)` pairs (0/1/63/64/65/127/128/129/1023/1025 and random) on every boundary length and pattern; `a = a.slice(l, r)` in mutation histories; byte sub-vector oracle. |
| `combineRange<Op>` | Exhaustive receiver/source lengths through 4 (5 in stress) with every mask pair, every `(l, r, p)` and all five ops, plus every aliased `(l, r, p)`; boundary lengths with same-size, exact-length, longer (random or maximal offset) and aliased sources; histories ops 15 (foreign source) and 17 (aliased). Oracle: per-bit `bitOp` on byte vectors. Mutations that drop the last edge word, the AVX2 carry word or the aliasing copy all fail the quick suite. |
| `Reference` copy | Copy-constructed proxy flips the shared bit; `vector<Bitset::Reference>` element assignment copies bit values; `-Werror` build. |
| Debug integration | `Debug::to_string(a[0])` spells `true`/`false`; `Debug::to_string(a)` equals the binary string for every lifecycle length. |

Quick mode exhausts unary states through length three and fused pairs through length two, then checks small word boundaries and 8 × 60 mutation steps. Full exhausts unary/pairs through length six (fused pairs through four), tests zero/one/alternating/boundary/random/sparse patterns through 8193 bits, and runs 32 × 180 history steps. Stress includes all full classes, extends boundaries to 32769 bits, and runs 96 × 500 history steps. Checked scalar assertions, optimized scalar without POPCNT/AVX, and AVX2+POPCNT optimized builds run in every mode; full/stress add scalar and AVX2 ASan/UBSan builds. The 16-word dispatch is covered on both sides and with partial final words.

The final count-kernel extraction and implicit-move return changes have a focused regression matrix, normally included in every mode. `--counts-only` explicitly runs that group with the selected mode’s build configurations; it is a focused run, not a claim of rerunning the entire full corpus. It checks whole/fused/range counts, 15/16/17 interior-word dispatch boundaries with unaligned endpoints, and the value-producing operators.

Optimized scalar uses `-O3 -DNDEBUG -march=x86-64 -mno-avx -mno-avx2 -mno-popcnt`; checked scalar uses `-O1 -g -D_GLIBCXX_ASSERTIONS` with the same ISA floor. Accelerated execution uses `-march=x86-64 -mavx2 -mpopcnt`. Both scalar and AVX2 sanitizer configurations use `-O1 -g -D_GLIBCXX_ASSERTIONS -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all -no-pie`. ASan, UBSan and LeakSanitizer all passed. The sanitizer subprocess limits are 600 s (full) and 1,200 s (stress); the sanitized stress run takes about five minutes. Range tests enumerate every boundary-point pair through 513 bits and sample twelve pairs plus the whole range above that, keeping the O(n) byte oracle within budget.

## Commands and results

Restyle, 2026-10-08 (`slice` comment reduced to its complexity line; behavior unchanged):

```bash
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/18-bitset.hpp
python3 '96-Local Testing/01-Core/18-bitset_tester.py' --mode full --seed 20261008
CXX=g++-14 python3 '96-Local Testing/01-Core/18-bitset_tester.py' --mode full --seed 20261008
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

All passed on GCC 16.2.1 and GCC 14.4.1 (20260915), GNU++20, Linux x86-64 (Intel Core i9-11900H), every build with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full (seed 20261008) ran **435,459,684 non-removable checks per configuration** in all five configurations (optimized scalar without POPCNT/AVX, checked scalar with precondition deaths, AVX2+POPCNT optimized, scalar and AVX2 ASan/UBSan with leak detection): 210.93 s on g++, 135.13 s on g++-14. `02-integration.py` passed 102 standalone/aggregate headers, scalar and AVX2 multiple-translation-unit linkage and the Workspace snapshot; `03-consistency.py` reports no errors. The benchmark below is from the 2026-10-06 re-audit; this restyle changed no code.

## Benchmarks

`96-Local Testing/01-Core/18-bitset_benchmark.jsonl` holds the 2026-10-06 run: GCC 16.2.1, `-O3 -DNDEBUG`, seed 20261006, five repetitions after doubling warmup to at least 3 ms, 142 checked measurements per configuration in baseline (no AVX/AVX2/POPCNT), POPCNT-only and AVX2 (`-mavx2 -mpopcnt -mno-avx512f`) configurations; all verification checks passed. It predates the review fix of the aliased `combineRange` branch, which no workload exercises. Data is uniform seeded random or one set bit per word; sizes include empty/single-word, 15/16/17-word dispatch neighbors, partial words, and lengths through 1,048,576 bits. Input construction/checking is excluded; subset sum includes construction, allocations and 96 seeded positive-weight transitions, verified by an independent byte DP. Generic references permit compiler vectorization, use native scalar POPCNT for counts, and implement shifts as ordinary word loops; materialized references use the value-returning operators. The run overlapped with the sanitized stress suite on another core, so absolute times are slightly pessimistic. Ratios are reference time over library time in the same AVX2 build.

| Operation / reference | 1,024 bits | 65,536 bits | 1,048,576 bits |
|---|---|---|---|
| Count / scalar POPCNT loop | 1.30× | 2.62× | 2.05× |
| Intersection count / fused scalar loop | 1.56× | 2.52× | 2.05× |
| Intersection count / materialized intersection | 2.62× | 1.95× | 2.27× |
| XOR / compiler-vectorized generic loop | 0.97× | 1.03× | 0.84× |
| Fused left-shift XOR / generic word loop | 2.46× | 5.50× | 2.99× |
| Range XOR `[13, n-11)` / materialized shift plus mask | 1.66× | 1.83× | 1.97× |
| Subset sum / temporary-producing shifts | 2.28× | 1.99× | 2.43× |

Threshold neighbours: whole count 0.93×/0.94× at 15 words (959/960 bits) and 1.28×/1.33× at 16 words (961/1,023 bits); range count `[13, n-11)` 0.88×–0.95× with 15 or fewer interior words (961–1,088 bits) and 1.25× at 16 interior words (1,152 bits); fused left shift 2.35×/1.80×/2.46× with 14–15 kernel words (960/961/1,024 bits, scalar path) and 2.84×/4.25×/4.48× with 16–18 kernel words (1,025/1,088/1,152 bits, AVX2 path). These confirm the retained 16-word dispatch for counts and for the shared shift kernel. Sparse one-bit-per-word data gives 2.29×/2.65× for count/intersection count at 65,536 bits. The 0.84× XOR figure at 1,048,576 bits compares two plain word loops that GCC vectorizes identically; it is memory-bound timing variation on a loaded host, not a kernel difference. Benchmarks remain shared-host observations, not timing gates.

## Sources

| Reference | Review and use |
|---|---|
| Installed GCC 16.2.1 libstdc++ `<bitset>` and `<tr2/dynamic_bitset>` | Fixed/dynamic word layouts, shifts, scan and padding contracts; implementation comparison, not imported code. |
| [Boost dynamic_bitset public source](https://raw.githubusercontent.com/boostorg/dynamic_bitset/develop/include/boost/dynamic_bitset/dynamic_bitset.hpp), SHA256 `e4902723b4b1f7be712ab3b42893ba97c1a929abf58c9ff983f0c882a8910e79` | Public API/comments inspected for sizing, access, conversions, set algebra, find and subset operations. |
| [suisen-cp dynamic_bitset](https://raw.githubusercontent.com/suisen-cp/cp-library-cpp/master/library/datastructure/util/dynamic_bitset.hpp), SHA256 `1461e85d264f362fd76371e0c53ed65f4c2f287e642a6cc979f949d1c2c1a1ae` | Full source inspected for packed and fused bit operations; its proxy masking and padding choices were not copied. |
| Muła, Kurz, Lemire, [Faster Population Counts Using AVX2 Instructions](https://arxiv.org/abs/1611.07612), v9, 2018-09-05, DOI `10.1093/comjnl/bxx046` | Abstract inspected for measured SIMD/Boolean-popcount motivation; no claim of full paper/proof review. Our nibble-table kernel was implemented independently and benchmarked locally. |

No external source code was copied into this header. No online submissions or online acceptance claims are part of this package.

The 2026-10-06 catalog sweep ([00-sources.md](00-sources.md), [00-notes.md](00-notes.md)) added `slice` and `combineRange` (maspypy `slice`, `or_to_range`/`xor_to_range`/`xor_suffix`; hitonanode `join`).

## Limits and handoffs

`combineRange` has no dedicated AVX2 edge handling (two masked scalar edge words plus the shared kernel); self-aliased `combineRange` allocates a copy of the source range. GCC 14.4.1 (`CXX=g++-14`, the floor check) also passed; exact GCC 14.2 and a Windows/MinGW build were not executed. BitMatrix (`12-bitmatrix.hpp`) may adopt `combineRange<Op::Xor>` for row elimination when its package runs.

## History

- 2026-09-27: original P002 completion, stress (1,004,660,940 checks per configuration), focused count matrix, integration and workspace check passed; first benchmark record.
- 2026-09-27: integer/style maintenance, full suite (282,326,372 checks per configuration) and integration passed.
- 2026-09-27: post-migration namespace review (renumbered 20 to 18), quick suite, consistency and workspace check passed.
- 2026-10-06: re-audit, 4 findings fixed (T/M line, `Reference` copy constructor, per-word assertion, closers), `slice` and `combineRange` added, `combineShift` moved onto `shiftWords`, full and stress passed on g++; benchmark rerun.
- 2026-10-06 (run): full (seed 20261006, 435,609,414 checks per configuration) and stress (seed 1, 1,412,889,926 checks) passed, `02-integration.py --sanitizers` passed, independent `@reviewer` oracle found no defect.
