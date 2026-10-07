# ST02 — static string hashing

## Research and scope

Inspected on 2026-09-28. These are algorithm and contract references; the implementation is independently written.

- [CP-algorithms, String Hashing](https://cp-algorithms.com/string/string-hashing.html), page update 2026-06-25: polynomial encoding, substring extraction, multiple moduli and the warning about adversarial collisions modulo `2^64`. The article's approximate `1/m` collision heuristic is not used as a rigorous guarantee here.
- Simon Lindholm, [KACTL Hashing.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/Hashing.h), dated 2015-03-15, CC0: prefix powers and interval subtraction, rolling windows, and the warning about Thue–Morse collisions in unsigned-word arithmetic. KACTL's separate `2^64-1` arithmetic is not adapted.
- [maspypy rolling_hash.hpp](https://github.com/maspypy/library/blob/main/string/rolling_hash.hpp): object-local base, powers, concatenation and substring LCP. Its modulus/reduction backend is not adapted.

Targeted searches of `OLD/algorithms.cpp`, `OLD/[1] algorithms.cpp`, `OLD/Team Notebook/src/algs.cpp` and `algsbetter.cpp` found container `CustomHash` implementations, but no rolling polynomial hash to migrate. Container hashing belongs to Miscellaneous. Archived originals remain unchanged.

Dynamic edits belong to ST09, multidimensional hashing to ST26, hash palindrome query adapters to ST03, and Rabin–Karp matching to ST08. This static header supplies reusable fingerprints for those owners. No online submission or acceptance is claimed.

## Collision model

For a fixed pair of distinct equal-length sequences of length L, injective symbol encoding makes their difference a nonzero polynomial of degree at most L-1 modulo each prime. If bases are sampled independently and uniformly from `[257,p-1]`, the root bound is `min(1,(L-1)/(p-257))` per prime. The product bounds a double collision; comparisons with different lengths reject exactly. A batch of comparisons uses a union bound. For LCP/LCS, bound the comparisons along the collision-free binary-search path: a wrong answer requires a first false-positive comparison on that path.

These assumptions require inputs independent of the random bases. A fixed default seed and deterministic seeded PRNG are conveniences for reproducibility, not a proof of independence or protection against adaptive adversaries. Unsigned modulo-`2^64` hashing has known structured collisions and receives no field root-bound guarantee. Equal fingerprints never certify sequence equality.

The length-1024 Thue–Morse binary word and its complement give a useful explicit limitation: their hash difference is, up to sign, `product(1-b^(2^i), i=0..9)`. For odd b, the factors contain at least 64 powers of two in total, so their unsigned-word fingerprints coincide despite unequal contents.

## API contract

`StringHash<>` uses primes 1,000,000,007 and 1,000,000,009 with independently selected bases. `StringHash64` uses unsigned arithmetic modulo `2^64` and an odd base. Both own their base, prefix powers and forward/reverse prefix fingerprints. There is no global base, cache, seed mutation or retained input reference. Default bases are fixed, and `randomBase(seed)` provides reproducible explicitly seeded alternatives. Objects with the same actual bases share a comparison context regardless of how those bases were obtained.

Inputs are byte strings (including embedded NUL and high bytes) or `vector<uint>` integer encodings, with length less than `INT_MAX`. Bytes encode as unsigned value plus one. The double-prime integer alphabet is `[0,1000000006)`; the word variant accepts every `uint`. Integer symbols also encode as value plus one. Arbitrary larger/signed alphabets can be compressed to consistent unsigned ranks before construction. Encoding must be consistent between compared sequences.

A digest contains its polynomial value, length and actual bases. Equality of unequal lengths or different bases returns false; equal digests remain fingerprints with the collision limitations above. `get(l,r)` and `reverseGet(l,r)` use half-open intervals, the latter hashing the reversal of exactly that interval. Empty intervals have zero value and length zero. `concat(a,b)` requires this object's bases on both inputs, returns the concatenation digest, and checks that total length fits `int`. Powers already stored are used directly; a right fragment longer than the object's input uses binary powering without growing hidden state.

`lcp(other,l,r,a,b)` and `lcs(other,l,r,a,b)` compare two substring ranges under equal bases and binary-search their common prefix or suffix length. The result is Monte Carlo because fingerprint equality can collide. No lexicographic order, exact equality, matching or palindrome predicate is claimed by these helpers.

Construction takes linear time and three linear arrays (powers, forward and reversed prefixes), each with eight-byte entries in either variant. Interval fingerprints cost constant time. Concatenation costs constant time when the right length is precomputed and logarithmic time otherwise. LCP/LCS take `O(log(k+1))` time for the smaller substring length k, with constant auxiliary space. Native constant-modulus arithmetic is used; there is no hand-written reducer, ISA specialization or timing-dependent dispatch.

## Correctness and verification

Horner prefixes satisfy `H[i+1]=H[i]*base+code(s[i])`. Subtracting `H[l]*base^(r-l)` from `H[r]` leaves the position-normalized substring polynomial. Reversed prefixes apply the same identity to indices `[n-r,n-l)`. Concatenation shifts the left polynomial by the right length. Binary powering covers lengths beyond the local table without adding mutable cache state. Prime residues remain normalized: sums stay below twice the modulus, and products fit `ulng`. The word variant intentionally uses defined unsigned wraparound.

The public state and arithmetic/build helpers expose contest implementation details; callers preserve their invariants. Digests passed to concatenation come from valid same-context fingerprints/concatenations. Copying owns independent arrays; after a move, assign a new object before querying the moved-from object. Allocation exhaustion propagates the standard allocation exception. No input storage is borrowed.

| Feature | Independent coverage |
|---|---|
| Forward/reverse interval fingerprints, empty intervals | Exhaustive binary strings, random integer symbols and Python arbitrary-precision sums of independently computed powers; every interval in each selected source. |
| Bytes/integer encoding and arithmetic | Embedded NUL, all 256 bytes, maximum allowed prime symbol, full `uint` word symbols, bases 257 and maximum allowed bases. |
| Concatenation and powers | Split/join identities, empty fragments, right fragments longer than the table, and a constructed `INT_MAX` digest followed by a checked overflow attempt. |
| LCP/LCS | Independent direct character scans, unequal lengths, empty/self ranges, independent objects, random and 200,000-byte unary/middle-mismatch inputs. |
| Contexts and value semantics | Copy/move construction, assignment, different-base and different-length rejection, seeded repeatability and a later object's independent base. |
| Collision limits | Ten seeded odd bases reproduce a known unequal 1024-byte Thue–Morse/complement collision in unsigned-word hashing. This is an expected limitation, not a failed exact-equality claim. |
| Preconditions | 13 checked assertion probes: base bounds/parity, integer alphabet, invalid ranges, mixed contexts, negative power and concatenated-length overflow. |

Full mode passed **833,559 C++ checks plus 5,504 Python exact interval checks in each of optimized NDEBUG, checked, and ASan/UBSan builds**, seed 20260928. All 13 assertion probes passed in the checked build. The first sanitizer execution was blocked by LeakSanitizer's refusal to run under the sandbox process tracer; the approved retry outside that tracer passed with leak checking enabled. Commands:

```bash
python3 '96-Local Testing/07-Strings/03-stringhash_tester.py' --mode full --seed 20260928
python3 '96-Local Testing/07-Strings/03-stringhash_tester.py' --mode full --seed 20260928 --configuration ASan-UBSan
```

Quick/full/stress exhaust binary strings through 4/7/9, add 20/150/1000 random sources, and use 10,000/200,000/1,000,000-byte large fixtures. The entry documents the remaining mode details and runs from any working directory. Stress is available but was not executed. Finite tests do not establish absence of hash collisions; the preceding algebra and probability assumptions define the actual guarantee.

## Performance evidence

The [benchmark entry](<../../96-Local Testing/07-Strings/03-stringhash_benchmark.py>) and [JSON record](<../../96-Local Testing/07-Strings/03-stringhash_benchmark.json>) compare double-prime hashing, unsigned-word hashing and direct byte LCP scans. Every warmup and timed answer was verified by direct scans. The record includes source/header hashes, raw repetitions and medians, setup/query/pipeline times, actual vector storage, bases, distributions and measurement exclusions.

```bash
python3 '96-Local Testing/07-Strings/03-stringhash_benchmark.py' --seed 20260928 --warmup 1 --reps 5
```

On the recorded Intel i9-11900H / GCC 16.2.1 host, GNU++20 `-O2 -DNDEBUG`, large sources had 262,144 bytes and 256 queries. Median complete construction-plus-query times in milliseconds were:

| Input | Double prime | Unsigned word | Direct scans |
|---|---|---|---|
| Random bytes | 2.381 | 1.355 | 1.825 |
| Period 11 | 2.541 | 1.336 | 5.507 |
| Unary | 2.503 | 1.323 | 6.946 |

The query distribution deliberately includes identical suffix starts in one eighth of queries, aligns some periodic phases, and independently caps other ends; these are not all independent random suffix comparisons. Small random/periodic inputs favor direct scans. Query-only times for the large hashed cases were about 0.064–0.070 ms for double primes and 0.045–0.047 ms for words, but setup is part of the pipeline comparison. Hash auxiliary storage was `80+24*(n+1)` bytes in this build (6,291,560 bytes at the large size), excluding allocator metadata and shared input/output harness arrays. Direct scanning uses constant auxiliary storage.

Unsigned-word speed does not remove its documented adversarial collisions. The double-prime default selects the stronger stated collision model. These shared-host timings impose no portability gate, universal winner or automatic dispatch threshold.
