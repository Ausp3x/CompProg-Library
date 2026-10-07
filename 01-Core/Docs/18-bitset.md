# C15 — dynamic packed bitset

The later [Core migration](../../00-Guidelines/History/2026-09-27-core-migration.md) moved the header and companion files from prefix 20 to 18. Commands and links below use current paths for reproduction; earlier results/hashes retain their historical meaning. The header bytes and raw benchmark JSONL are unchanged, while tester/benchmark includes now use the current header path. The post-migration review at the end records current source hashes and fresh checks.

`18-bitset.hpp` provides `Bitset`, a runtime-length array of bits backed by contiguous 64-bit words. C15 follows [C01](01-template.md) in P002. The implementation is independent of BitMatrix and has no runtime CPU dispatch. All arithmetic and indexing use the GNU C++20 x86-64 contract (Linux and Windows/MinGW, see [01-template.md](01-template.md#platform-and-includes)).

## Supported API and domain

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

## Contracts

### Bitset

Runtime length `n`, bit 0 least significant, ranges half-open `[l, r)`. Boolean operations, `isSubsetOf` and `intersects` require equal sizes; equality does not. Shifts by at least `size()` yield zero; rotations reduce modulo `size()` and leave an empty set unchanged. Scans return `size()` on absence; `findNext`/`findPrev` are strictly after/before `p <= size()`. Public `n` and `a` are representation, not mutation APIs: `a.size() == ceil(n / 64)` with zero padding. Proxies and block spans invalidate on resize, clear, push/pop, move, assignment and swap; other mutations preserve them, and scans keep no persistent iterators. Not thread-safe. Every mutating operation supports self-aliasing; moved-from objects are empty. Resize costs worst-case O(old words + new words) on growth and O(1) on shrink; `pushBack` is amortized O(1); `popBack`, `clear`, `swap`, `size`, `empty` and `blocks` are O(1). Rotation, copies and nonmutating operators use O(ceil(n / 64)) extra result or workspace.

### shiftWords

`dst[i] OP= word i of x shifted by s < 64` for `i` in `[0, words)`. Left reads `x[i]`, `x[i - 1]` descending; right reads `x[i]`, `x[i + 1]` ascending. The outer neighbour (`x[-1]` or `x[words]`) is read only when `s != 0`. Each group loads its sources before storing, so `dst` may overlap `x` in the direction of travel.

### combineShift

`this OP= (b shifted k)` with equal sizes; `b` may alias `this` with original-value semantics, still in O(1) workspace.

### combineRange

`this[l, r) OP= b[p, p + (r - l))`; sizes may differ, and when `b` aliases `this` its source range is copied first, costing O(1 + (r - l) / 64) workspace.

## Complexity and correctness argument

Let `W = ceil(n / 64)`. Bulk operations and scans take worst-case `O(W)` time; scans and Boolean predicates stop early when possible. Single-bit access, size, clear, swap and span access take constant time. Range work takes `O(1 + (r-l)/64)`. Growing resize takes worst-case `O(old W + new W)` if allocation moves existing words; pushBack is amortized constant time. Shrink/pop are constant time. Text import/export take `O(n)` time. Stored memory is `O(W)`; text export uses `O(n)` returned memory, copying/nonmutating operators and rotation use `O(W)` result/workspace, and fused mutations use constant workspace.

Bit position `i` is word `i/64`, offset `i%64`. Splitting each shift into whole-word displacement `d` and a residual `s` in `[0,63]` gives the two contributing words. The shared kernel `shiftWords<Op>(dst, x, words, s, left)` applies `dst[i] OP= x[i] << s | x[i-1] >> (64-s)` (left, descending) or `dst[i] OP= x[i] >> s | x[i+1] << (64-s)` (right, ascending) and reads the outer neighbour `x[-1]`/`x[words]` only when `s != 0`; a residual-zero branch avoids scalar shifts by 64. `combineShift` passes the interior words so that both neighbours exist and finishes the single boundary word (`a[d]` for left, `a[size-d-1]` for right) after the kernel, which preserves the descending/ascending order that self-aliasing needs. Each SIMD group loads all contributing source words before writing its four destinations, so the same proof applies to self-aliasing. `combineRange` masks its two edge words with guarded source reads (missing source words read as zero, which only affects masked-out bits because every in-range destination bit maps into `[p, p + r - l)`) and hands the fully-covered interior words to `shiftWords`; for interior words both source neighbours are in bounds because the lowest interior bit maps to a source position `>= p >= 0` and the highest to a position `< p + (r - l) <= b.n`. Aliased calls copy the source range first, so no read sees a modified word. Vacated words are cleared for assignment/AND, and skipped for OR/XOR/AND-NOT. Range count masks its two endpoint words and passes the complete interior words to the same scalar/AVX2 count kernel. All paths mask the last word after shifts, rotations and length-changing operations. Whole-word Boolean operations preserve zero padding. These invariants justify scans, count, equality and subset checks without inspecting storage past its end.

Word counts use division plus a remainder test, without overflowing `n + 63`. Shift amounts are compared against `n` before displacement arithmetic. Range masks handle 0 and 64 explicitly. Scan sentinels never collide with valid indices. The AVX2 popcount uses duplicated nibble tables, byte counts at most eight, and 64-bit SAD reductions, so there is no byte accumulator overflow.

## Optimization scope and provenance

The baseline uses ordinary word arithmetic and `std::popcount`, with no required ISA flags. `__AVX2__` selects unaligned four-word directional fused shift and nibble-table population-count kernels, including fused Boolean counts; scalar tails handle every alignment and length. These kernels start at 16 words of kernel work (whole counts from 961 bits; for fused shifts the count excludes the boundary word, so from 1,025 bits with `k < 64`). Plain Boolean loops rely on compiler vectorization: the initial explicit AVX2 loop was measurably slower and was removed. Benchmark evidence below records the final selection. FMA and floating-point flags are irrelevant to exact bit operations. BMI-specific scan instructions can be emitted by the compiler when the caller enables them; the implementation requires neither BMI nor POPCNT.

The scope review added word/string conversion, subset/intersection queries and counts, last-bit scans, proxy assignment, and explicit move semantics to the starting inventory. Fixed-size `std::bitset` remains the compact alternative. Growable word append, formatted stream extraction, hashing/ordering, unset-bit scans and proxy compound assignment are convenience extensions rather than promised C15 features; they can be composed from the supported API. Succinct rank/select, compressed/Roaring bitmaps and sparse set representations belong to Data Structures. AVX512 VPOPCNTDQ and Harley–Seal carry-save population count are alternative backend research, not implemented or claimed verified here; this package does not promise every ISA or every published popcount backend.

References inspected on 2026-09-27:

| Reference | Review and use |
|---|---|
| Installed GCC 16.2.1 libstdc++ `<bitset>` and `<tr2/dynamic_bitset>` | Fixed/dynamic word layouts, shifts, scan and padding contracts; implementation comparison, not imported code. |
| [Boost dynamic_bitset public source](https://raw.githubusercontent.com/boostorg/dynamic_bitset/develop/include/boost/dynamic_bitset/dynamic_bitset.hpp), SHA256 `e4902723b4b1f7be712ab3b42893ba97c1a929abf58c9ff983f0c882a8910e79` | Public API/comments inspected for sizing, access, conversions, set algebra, find and subset operations. |
| [suisen-cp dynamic_bitset](https://raw.githubusercontent.com/suisen-cp/cp-library-cpp/master/library/datastructure/util/dynamic_bitset.hpp), SHA256 `1461e85d264f362fd76371e0c53ed65f4c2f287e642a6cc979f949d1c2c1a1ae` | Full source inspected for packed and fused bit operations; its proxy masking and padding choices were not copied. |
| Muła, Kurz, Lemire, [Faster Population Counts Using AVX2 Instructions](https://arxiv.org/abs/1611.07612), v9, 2018-09-05, DOI `10.1093/comjnl/bxx046` | Abstract inspected for measured SIMD/Boolean-popcount motivation; no claim of full paper/proof review. Our nibble-table kernel was implemented independently and benchmarked locally. |

No external source code was copied into this header. No online submissions or online acceptance claims are part of this package.

## Verification and benchmark evidence

The independent byte-per-bit oracle lives in [18-bitset_tester.cpp](<../../96-Local Testing/01-Core/18-bitset_tester.cpp>), with the runnable [Python entry](<../../96-Local Testing/01-Core/18-bitset_tester.py>). The entries accept `--mode quick|full|stress`, `--seed`, and the shared runner’s `CP_TEST_MODE/CP_TEST_SEED`. They run from an unrelated working directory, retain checks under `-DNDEBUG`, fail on subprocess/timeout errors, and report unavailable ISA execution as SKIP.

| Public feature | Independent coverage |
|---|---|
| Construction, word/string import/export, debug hook, size/empty | Per-bit reconstruction, MSB-first strings, dirty imported padding, empty and exact/partial words, LOCAL debug across translation units. |
| Access/proxies, set/reset/flip, ranges and range masks | Exhaustive small patterns/ranges, first/last bits, all word boundaries, stable proxy assignment/flip and mutation histories. |
| Count/any/none/all, scans | Byte sums and independently scanned positions, empty/zero/all-one/sparse/random patterns, strict scan endpoints and sentinel. |
| Boolean operators and compounds, complements, equality, subset/intersection and fused counts | Exhaustive input pairs through length six, byte Boolean oracles, all self-alias cases, unequal-size equality and checked preconditions. |
| Shifts/rotations and all five fused operations | Exhaustive tiny inputs, both directions, self/distinct source, 0/63/64/65/length/SIZE_MAX amounts, word/AVX2/dispatch neighbors and nonzero tails. |
| Resize, clear/reuse, push/pop, copy/move/swap | Shrink/regrow with both fill values, growth through multiple words, unchanged-size resize, source emptiness after move, self-copy/move/swap and randomized operation histories. |
| Preconditions | 28 separate assertion-death cases: access, malformed ranges/string/word count, scan position, empty pop, and every equal-size operation family. |

Quick mode exhausts unary states through length three and fused pairs through length two, then checks small word boundaries and 8 × 60 mutation steps. Full exhausts unary/pairs through length six (fused pairs through four), tests zero/one/alternating/boundary/random/sparse patterns through 8193 bits, and runs 32 × 180 history steps. Stress includes all full classes, extends boundaries to 32769 bits, and runs 96 × 500 history steps. Checked scalar assertions, optimized scalar without POPCNT/AVX, and AVX2+POPCNT optimized builds run in every mode; full/stress add scalar and AVX2 ASan/UBSan builds. The 16-word dispatch is covered on both sides and with partial final words.

Original package integration passed all 58 then-current standalone/aggregate headers, scalar and AVX2 multiple-translation-unit linkage, and workspace compilation. A separate LOCAL Basic/All two-translation-unit check exercised the `debugString(Bitset)` hook. The workspace generator fixture passed after accepting debug’s canonical direct include; explicit snapshot generation and staleness checking passed. The tests use GCC 16.2.1 on Linux x86-64; GCC 14 itself was unavailable, so there is no claim of execution on that exact compiler version.

The final count-kernel extraction and implicit-move return changes have a focused regression matrix, normally included in every mode. `--counts-only` explicitly runs that group with the selected mode’s build configurations; it is a focused run, not a claim of rerunning the entire full corpus. It checks whole/fused/range counts, 15/16/17 interior-word dispatch boundaries with unaligned endpoints, and the value-producing operators.

### Historical performance measurements — original P002

```bash
python3 '96-Local Testing/01-Core/18-bitset_benchmark.py' --seed 20260927 \
  --milliseconds 3 --repetitions 5 --output /tmp/p002-bitset-benchmark.jsonl
```

The `96-Local Testing/01-Core/18-bitset_benchmark.jsonl` contains 124 checked measurements per configuration, including exact compiler commands, checksums, environment and input distributions. Historical benchmark header SHA256: `33e28c58b7fe1f6ffae3909f3ea3d84e4258130e46288e381d931b1c816bd201`.

The host reports Intel Core i9-11900H, Linux x86-64, GCC 16.2.1 (20260810). All configurations use GNU++20, `-O3 -DNDEBUG`; baseline disables AVX/AVX2/POPCNT, native-word count enables POPCNT only, and AVX2 uses `-mavx2 -mpopcnt -mno-avx512f`. Data is uniform seeded random or one set bit per word. Sizes include empty/single-word, 15/16/17-word dispatch neighbors, partial words, and lengths through 1,048,576 bits. Each result is the median of five trials after doubling warmup iterations to at least 3 ms. Input construction/checking is excluded from kernel timings. Subset sum includes construction, allocations and 96 seeded positive-weight transitions; its independent byte DP verifies the result. A compiler memory barrier prevents hoisting and contributes to tiny-input cost. Stored arrays use `8 * ceil(n/64)` bytes each; fused shifts use constant workspace and avoid the shifted temporary.

The table shows reference time divided by library time in the **same AVX2 build**. Generic references permit compiler vectorization, use native scalar POPCNT for counts, and implement shifts as ordinary word loops. Materialized references use the corrected value-returning operators (one input copy, then move of the result); earlier measurements with an avoidable return copy are superseded.

| Operation / reference | 1,024 bits | 65,536 bits | 1,048,576 bits |
|---|---|---|---|
| Count / scalar POPCNT loop | 1.32× | 2.30× | 2.12× |
| Intersection count / fused scalar loop | 1.53× | 2.58× | 2.10× |
| Intersection count / materialized intersection | 2.53× | 1.97× | 2.25× |
| XOR / compiler-vectorized generic loop | 0.97× | 0.98× | 0.99× |
| Fused left-shift XOR / generic word loop | 2.81× | 3.74× | 2.78× |
| Subset sum / temporary-producing shifts | 2.54× | 1.76× | 2.27× |

Large unaligned range counts measured 2.50×/2.28× at 65,536/1,048,576 bits. At 15/16/17 *interior* words, range-count ratios were 0.92×/1.20×/1.21×; together with whole-count threshold measurements these support the retained 16-word dispatch. The initial explicit Boolean AVX2 loop lost to compiler vectorization; the final plain loop is within ordinary timing variation of its generic counterpart. Benchmarks are shared-host observations, not a universal fastest-implementation claim or a hard timing gate. All baseline, POPCNT and AVX2 benchmark correctness checks passed.

### Original P002 completion checks

```bash
python3 '96-Local Testing/01-Core/18-bitset_tester.py' --mode quick --seed 42
python3 '96-Local Testing/01-Core/18-bitset_tester.py' --mode stress --seed 20260927
python3 '96-Local Testing/01-Core/18-bitset_tester.py' --mode full --seed 20260927 --counts-only
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/00-Tools/02-online_tester.py'
python3 '97-Online Testing/03-workspace.py' --check
```

All passed. Quick exercised the three non-sanitizer configurations. Broad stress exercised the complete full feature corpus plus its documented extensions: **1,004,660,940 non-removable checks per configuration**, five configurations, 28 assertion-death contracts, 474.02 seconds total. The subsequent localized count-kernel reuse and removal of redundant value-return copies passed the persistent focused matrix: **1,312,059 checks per configuration**, all five configurations, 28.21 seconds. That final group also checks materialized operators with lvalue/moved operands and moved-from emptiness. The final full configuration matrix is thus covered by broad stress plus focused validation of the last two local changes, rather than a claim that the entire billion-check corpus was repeated after them.

Optimized scalar uses `-O3 -DNDEBUG -march=x86-64 -mno-avx -mno-avx2 -mno-popcnt`; checked scalar uses `-O1 -g -D_GLIBCXX_ASSERTIONS` with the same ISA floor. Accelerated execution uses `-march=x86-64 -mavx2 -mpopcnt`. Both scalar and AVX2 sanitizer configurations use `-O1 -g -D_GLIBCXX_ASSERTIONS -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all -no-pie`. ASan, UBSan and LeakSanitizer all passed. The initial sandboxed leak check could not use ptrace; approved execution outside the sandbox resolved that environment limitation. No sanitizer timeout or unresolved failing reproducer remains.

P002’s owned C01 and C15 features and required verification are complete. The documented API exclusions and unexecuted exact GCC14 version are scope/platform limits, not hidden implemented-feature claims. No handoff is needed for owned gaps.

## Integer and style maintenance — 2026-09-27

The revised integer/style guide and completed-package maintenance workflow were applied only to C15’s owned header and C++ test/benchmark support. Packed storage, masks, scan words, SIMD lanes, pointers/spans, seed values and hash accumulators now use the exact `ulng` alias for `uint64_t`; no word arithmetic changed signedness. Methods are separated into logical groups, unrelated setup/mutation/branch steps are split, and local names are shortened consistently. Public names/types, data-member and initializer order, algorithms, thresholds and supported domains are preserved.

Bitset’s public size/index/count/shift interfaces and matching word offsets remain `size_t`, including `SIZE_MAX` shifts and the `size()` scan sentinel. Existing helper signatures and generic oracle/benchmark size domains retain that type as well; a container’s unsigned `size()` alone was not used as the justification. Iterator offsets retain `ptrdiff_t`. The local residual shift uses `int(k % 64)`, whose value is proven to lie in `[0,63]`; the original full-width shift amount remains `size_t` and is checked before displacement arithmetic.

Bounded test drivers, dimension lists, prefixes and indices use `int`: exhaustive dimensions are at most six; named stress lengths are at most 32769; count-matrix lengths are at most 4097; mutation histories start/reset at at most 4097 and permit at most 500 pushes, so their lengths are at most 4597. Exhaustive bit masks remain unsigned, using `uint`. The run-wide check counter uses `lng`. Benchmark fixture sizes are at most 1,048,576 and adaptive timing counts stop at `2^26`, so these counters use `int`. General oracle/benchmark helpers still accept `size_t` sizes/counts, and shift/rotation test operands retain the full unsigned domain. Random draws, seeds, tested cases and benchmark workloads are unchanged.

### Verification at integer/style maintenance

```bash
python3 '96-Local Testing/01-Core/18-bitset_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/02-integration.py'
```

Both passed with GCC 16.2.1. The complete all-feature full suite ran **282,326,372 non-removable checks per configuration**, all five scalar/AVX2 configurations, in 147.09 seconds; checked scalar rejected all 28 precondition cases. Scalar and AVX2 ASan/UBSan/LeakSanitizer passed. An initial sandbox run completed scalar sanitizer checks but LeakSanitizer could not use ptrace at exit; the unchanged suite passed in an approved run outside that sandbox with leak checking retained. The integration runner passed all 58 present standalone/aggregate headers, scalar and AVX2 multiple-TU linkage, and compilation of the existing Workspace snapshot. Benchmark C++ compiled with GNU++20, `-O3 -DNDEBUG -Wall -Wextra` in baseline (no AVX/AVX2/POPCNT), POPCNT-only, and AVX2+POPCNT/no-AVX512 configurations. No fresh benchmark timing run or performance comparison was made; the historical table and raw JSONL above retain their original provenance and are not measurements of the maintained source.

Source hashes at integer/style maintenance, before renumbering (SHA256; paths shown with their current prefix for navigation):

| File | SHA256 |
|---|---|
| `01-Core/18-bitset.hpp` | `01150bfcbf3638f8407bb3e19ad6c421c3e47b61c32ee37df3b610c39b640bd4` |
| `96-Local Testing/01-Core/18-bitset_tester.cpp` | `e12823abacd171f857639eaebacb6848cdbf5f5d5f216b8e4a62aaf3998d7a2e` |
| `96-Local Testing/01-Core/18-bitset_benchmark.cpp` | `fbf25187dc9e7c8bb843f65c7a90ff0adb60493c11dc84101b344608839bbcff` |

The historical JSONL report remains byte-for-byte unchanged, SHA256 `ad8aeabd36afb166e678e601959bf5e5f8b37d40058d6d5da3311270def5cd21`; its benchmark header hash remains `33e28c58b7fe1f6ffae3909f3ea3d84e4258130e46288e381d931b1c816bd201`. Recompiling the benchmark is compilation evidence only. Original stress results were not rerun or relabeled as fresh maintenance results.

C01’s [Workspace handoff](02-debug.md#workspace-handoff-to-sup05p003) recorded a stale generated snapshot for SUP05/P003 at that stage. P003 and the subsequent Core migration resolved it; the post-migration review below confirms the current snapshot.

## Post-migration namespace review — 2026-09-27

Reviewed C15 after C01 against the current C++ guide, completed-package maintenance workflow, inventory/batch rows and Core migration record. `18-bitset.hpp` already satisfies the integer, namespace and grouping rules and remains byte-for-byte unchanged. The benchmark's anonymous namespace now has four-space body indentation and an own-line closing comment. Its C++ tokens, workloads, random draws and arithmetic are unchanged. The C++ tester and Python entries needed no edit; their includes, companion discovery, quick-runner entry and documentation use the current `18-bitset` paths.

The documented `size_t` size/count/shift domains and generic helper interfaces, `ptrdiff_t` iterator offsets, exact `ulng` words and unsigned mask/seed/hash arithmetic remain justified as described above. Bounded drivers still use `int`, and the run-wide check counter remains `lng`. Public APIs, domains, brace style and member/initializer order are preserved.

Fresh checks with GCC 16.2.1, GNU++20, Linux x86-64:

```bash
python3 '96-Local Testing/01-Core/18-bitset_tester.py' --mode quick --seed 20260927
python3 '96-Local Testing/03-consistency.py'
python3 '97-Online Testing/03-workspace.py' --check
```

The quick suite passed optimized scalar without POPCNT, checked scalar, and AVX2+POPCNT: **13,049,862 non-removable checks per configuration**, all 28 expected precondition failures, 18.09 seconds total. The final benchmark source compiled with `-std=gnu++20 -O3 -DNDEBUG -Wall -Wextra` in baseline (`-mno-avx -mno-avx2 -mno-popcnt`), POPCNT-only (`-mno-avx -mno-avx2 -mpopcnt`), and AVX2 (`-mavx2 -mpopcnt -mno-avx512f`) configurations. No benchmark timing run was made for this layout change. The earlier full/stress, sanitizer, integration and timing results remain historical evidence rather than fresh results for this review.

Consistency validation passed with 428 targets, 335 batches and 226 packages, including current includes, test discovery and links. Workspace `--check` passed without modifying the snapshot. There is no remaining P002-owned gap or stale SUP05/P003 snapshot; no template/debug input changed in this review.

Current source hashes (SHA256):

| File | SHA256 |
|---|---|
| `01-Core/18-bitset.hpp` | `01150bfcbf3638f8407bb3e19ad6c421c3e47b61c32ee37df3b610c39b640bd4` |
| `96-Local Testing/01-Core/18-bitset_tester.cpp` | `7c837865d25daf602bbd598413f42a4b3b585f9e772c5e04d05f71128c13d9e9` |
| `96-Local Testing/01-Core/18-bitset_benchmark.cpp` | `c8aa468a0d219dc02752080b3cb34b8ceb348333fc93a1e0a663b185ee753f07` |

The raw benchmark JSONL still has SHA256 `ad8aeabd36afb166e678e601959bf5e5f8b37d40058d6d5da3311270def5cd21`. Its historical paths and measured header hash were preserved. Shared inventories/checklists, Workspace, other implementations and legacy originals were not edited.

## Re-audit — 2026-10-06

Package P002 re-audit under the current rules; the previous verification was treated as existing-unverified. Operations in the inventory row were compared with the code and the tester before any edit, the full suite was rerun unchanged (five configurations, 139.40 s, passing), and the header was then brought to the current style, extended and re-tested.

### Confirmed findings and their disposition

| # | Finding | Disposition |
|---|---|---|
| 6 | T/M line not directly above `struct Bitset` | Fixed: the bound line is the last comment line above the struct. |
| 7 | `Reference` declared copy assignment without a copy constructor (`-Wdeprecated-copy`) | Fixed: explicit `Reference(ulng *, ulng)` and `Reference(const Reference &) = default`. The tester copies named proxies, stores proxies in a `vector<Bitset::Reference>` and compiles with `-Werror`; removing the defaulted constructor fails the build (mutation check). |
| 8 | Per-word `assert` inside the fused-shift inner loop | Fixed: `shiftedWord` is gone; the shared `shiftWords` kernel has no assertion and `combineShift`/`combineRange` check their preconditions once at entry. |
| — | `; }` closers of multi-line blocks (lines 118, 137, 218, 224, 240, 245 of the previous header) | Fixed; the tester and benchmark were normalized mechanically, and `03-consistency.py --braces` reports nothing for the three files. |

### Changes beyond the findings

- New operations from the catalog sweep ([00-sources.md](00-sources.md), [00-notes.md](00-notes.md)): `slice(l, r)` and `combineRange<Op>(l, r, b, p)` (maspypy `slice`, `or_to_range`/`xor_to_range`/`xor_suffix`; hitonanode `join`). `combineRange` reuses the five `Op` kernels and `shiftWords`, so the AVX2 path serves offset ranges as well; aliasing copies only the source range (the review found the first version copied the whole set, an O(ceil(n / 64)) cost per call, confirmed with 20,000 aliased 64-bit XORs on a 2^24-bit set taking 2.7 s against under 1 ms through `slice`).
- `combineShift` now delegates to `shiftWords` and handles one boundary word explicitly; the result bits are unchanged (the existing exhaustive/boundary/history oracles passed before and after). The AVX2 dispatch counts kernel words rather than total words, which moves the fused-shift crossover from 961 to 1,025 logical bits for `k < 64`; the benchmark below re-measures both sides.
- The tester includes the LOCAL debug header to check `Debug::to_string(Bitset::Reference)` and the `debugString` hook end to end, uses `-Werror`, and adds six precondition deaths (`combine-range-order/end/source/offset`, `slice-order/end`, 34 in total).

### Feature-to-test additions

| Feature | Independent coverage |
|---|---|
| `slice` | Exhaustive all `(l, r)` for every pattern through length 4 (5 in stress); word-boundary `(l, r)` pairs (0/1/63/64/65/127/128/129/1023/1025 and random) on every boundary length and pattern; `a = a.slice(l, r)` in mutation histories; byte sub-vector oracle. |
| `combineRange<Op>` | Exhaustive receiver/source lengths through 4 (5 in stress) with every mask pair, every `(l, r, p)` and all five ops, plus every aliased `(l, r, p)`; boundary lengths with same-size, exact-length, longer (random or maximal offset) and aliased sources; histories ops 15 (foreign source) and 17 (aliased). Oracle: per-bit `bitOp` on byte vectors. Mutations that drop the last edge word, the AVX2 carry word or the aliasing copy all fail the quick suite. |
| `Reference` copy | Copy-constructed proxy flips the shared bit; `vector<Bitset::Reference>` element assignment copies bit values; `-Werror` build. |
| Debug integration | `Debug::to_string(a[0])` spells `true`/`false`; `Debug::to_string(a)` equals the binary string for every lifecycle length. |

### Commands and results

```bash
python3 '96-Local Testing/01-Core/18-bitset_tester.py' --mode full --seed 20261006
python3 '96-Local Testing/01-Core/18-bitset_tester.py' --mode stress --seed 1
python3 '96-Local Testing/01-Core/18-bitset_benchmark.py' --seed 20261006 --milliseconds 3 --repetitions 5 --output '96-Local Testing/01-Core/18-bitset_benchmark.jsonl'
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

All passed on GCC 16.2.1, GNU++20, Linux x86-64 (Intel Core i9-11900H), every build with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Before the edits the unchanged suite passed full mode (seed 20260927, five configurations, 139.40 s). After the edits, full (seed 20261006) ran **435,609,414 non-removable checks per configuration** in all five configurations (optimized scalar without POPCNT/AVX, checked scalar with 34 precondition deaths, AVX2+POPCNT optimized, scalar and AVX2 ASan/UBSan with leak detection), 216.32 s total for the final source (246.59 s for the pre-review source); stress (seed 1) ran **1,412,889,926 checks per configuration**, five configurations, 731.21 s. The sanitizer subprocess limits were raised to 600 s (full) and 1,200 s (stress) because the sanitized stress run of the enlarged corpus takes about five minutes; the limits still catch hangs. Range tests enumerate every boundary-point pair through 513 bits and sample twelve pairs plus the whole range above that, keeping the O(n) byte oracle within budget.

`python3 '96-Local Testing/02-integration.py' --sanitizers` passed: 102 standalone/aggregate headers, scalar and available AVX2 multiple-translation-unit linkage, the regenerated Workspace snapshot and the sanitizer self-tests. `python3 '96-Local Testing/03-consistency.py'` reports no errors.

Independent review (`@reviewer`, 2026-10-06) confirmed every finding fixed, re-derived the kernel bounds, ran its own brute-force oracle (sizes to 3,000, differing source sizes, all ops, both directions, aliasing, ASan/UBSan scalar and AVX2) and the full suites, and raised: the whole-set copy on aliased `combineRange` (fixed as above), missing public helpers in the row (added), and brace-checker edge cases (fixed; see [00-notes.md](00-notes.md#closing-brace-checker)).

### Benchmark — 2026-10-06

Rerun of the benchmark on the maintained source before the review fix of the aliased `combineRange` branch, which no benchmark workload exercises (the `96-Local Testing/01-Core/18-bitset_benchmark.jsonl` now holds this run, SHA256 `0425f73f87432704365c7b799fcb43ed8bdb4ebfd61f0337f9503141ea36c98d`; the previous record's figures stay in the historical table above). Same host, GCC 16.2.1, `-O3 -DNDEBUG`, seed 20261006, five repetitions, 3 ms adaptive warmup, 142 checked measurements per configuration in baseline, POPCNT-only and AVX2 configurations; all verification checks passed. The run overlapped with the sanitized stress suite on another core, so absolute times are slightly pessimistic while the interleaved reference/library ratios below are unaffected in kind. Ratios are reference time over library time in the same AVX2 build.

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

Source hashes after the re-audit (SHA256):

| File | SHA256 |
|---|---|
| `01-Core/18-bitset.hpp` | `79a8680ebc3629c5a1e58d028b0c62ce941be6af53de720a84211e4481ac7924` |
| `96-Local Testing/01-Core/18-bitset_tester.cpp` | `61649ba856eeacdeaf897b649d7dbf7218e4e218e3867e0dec56e0cabb39b58c` |
| `96-Local Testing/01-Core/18-bitset_tester.py` | `905dbbecd5daf5c5ab0a128e67017bbc3d73f2437839bb5b4a1f7bc9075d5a82` |
| `96-Local Testing/01-Core/18-bitset_benchmark.cpp` | `a9b076aaf73ab3dabbed74c8f2421fe5e73ed30bac79c93c15f7dbfbeaa757e1` |
| `96-Local Testing/01-Core/18-bitset_benchmark.jsonl` | `0425f73f87432704365c7b799fcb43ed8bdb4ebfd61f0337f9503141ea36c98d` |

Known limits and handoff: `combineRange` has no dedicated AVX2 edge handling (two masked scalar edge words plus the shared kernel); self-aliased `combineRange` allocates a copy of the source range. GCC 14 and Windows were not executed. BitMatrix (`12-bitmatrix.hpp`) may adopt `combineRange<Op::Xor>` for row elimination when its package runs; no P002-owned gap remains.
