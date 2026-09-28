# Random and custom hash — MI01 verification

Completed 2026-09-27 for P013. `01-random.hpp` and `02-customhash.hpp` cover all owned Basic inventory features. Advanced random sampling and universal/tabulation/adversarial-resistant hash families remain owned by MI05; they are not claims of these headers.

## Random API and argument

`Random()` seeds `std::mt19937_64` from `steady_clock` ticks; this is a convenient varying seed, not an entropy source with guaranteed entropy or uniqueness. `Random(ulng)` and `seed(ulng)` accept the entire unsigned 64-bit seed domain. Record an explicit seed to replay. The public `rng` engine remains available, and the archived monolith's `inline Random rng` convenience object is restored. Copy/move preserve engine state. Single-threaded use is the baseline; callers synchronize shared objects if needed.

`randInt(l,r)`, `randLng(l,r)` and `randUlng(l,r)` use **inclusive** bounds and support their entire representable integer domains, including singleton endpoints and the complete signed/unsigned 64-bit intervals. Preconditions are `l <= r`. `randBelow(n)` requires `n > 0` and returns `[0,n)`. Bounds are asserted. Violations remain invalid when assertions are disabled.

The mapping is Lemire's multiply/reject method. Put `M=2^64`, `t=M mod n`, and `p=x*n`. Accept when `p mod M >= t`, then return `floor(p/M)`. Every output in `[0,n)` has exactly `floor(M/n)` accepted preimages; conditioning on acceptance therefore adds no bias to uniform independent input words. At least half the words are accepted, so expected work is less than two draws; rejection has no finite worst-case bound under that ideal model. Testing a deterministic PRNG does not prove independent ideal randomness, and MT is not cryptographic.

Unsigned subtraction computes interval width modulo `2^64`; zero width denotes exactly the full domain. Signed offsets are added in signed 128-bit arithmetic and only converted after the result is known to fit. Thus neither a signed subtraction nor an overflowing signed addition is used. Engine generation/state has a fixed 312-word cost; bounded operations have expected O(1) work and O(1) state with the word size fixed.

`shuffle(first,last)` accepts a valid mutable random-access iterator range (including proxy/move-only elements), uses O(1) workspace and expected O(n) time, and consumes no words for sizes zero or one. The range must not be reversed. Fisher–Yates chooses each final position uniformly among remaining elements; the sequence of choices is in bijection with the `n!` labeled permutations. Repeated equal values retain their multiplicities. Iterators and element swaps have their normal standard-library preconditions. The standard engine plus explicitly implemented interval mapping and shuffle order give replay independent of `std::uniform_int_distribution` or `std::shuffle` implementation choices. Existing legacy `randInt`/`randLng` domains are preserved; their old implicit sequences were never stable API results.

## Hash API, equality and collision limits

`CustomHash()` copies one inline process seed `CustomHash::rnd`, formed from clock ticks and that object's address. Address variation is platform/process dependent and no secrecy, uniqueness or entropy bound is promised. `CustomHash(ulng)` supplies a reproducible explicit seed. Hash results replay on the supported Linux x86-64 platform; string chunks use that platform's little-endian representation. A hasher's seed must remain fixed while it serves a container. Copies and container rehashes retain it.

The scalar SplitMix64 increment/finalizer is a permutation: addition modulo `2^64`, xor-right shifts and multiplication by odd constants are individually invertible. For a fixed seed, distinct 64-bit integer representations therefore have distinct full 64-bit hash values. Both words of signed/unsigned 128-bit keys are included. Scoped enums are added; bool, smaller integers, unscoped enums and legacy implicitly-convertible values remain supported. Nonintegral/user-defined conversions must be defined and compatible with equality; in particular NaN/out-of-range floating conversion is invalid, signed zeros hash equally, and fractional values may collide after truncation. `CustomHash::ulng` now uses the repository's exact `::ulng` alias.

Pair/tuple and recursively nested ordered ranges mix child hashes in order; tuples/ranges also mix their length. Empty tuples/ranges are supported. Range adapters accept const-iterable input ranges whose equality respects the same visited sequence. They must not be paired with order-insensitive equality, such as an unordered container used as a key. Scalar work is O(1); strings and composites take O(total visited bytes/elements), with O(nesting depth) transient stack and O(1) stored hasher state. String chunks use `memcpy` so unaligned buffers are valid, and tails are zero-padded before their exact byte length is mixed.

`string`, `string_view` and NUL-terminated C-string **arrays/literals** hash text. Embedded NUL bytes are included by strings and explicit-length views; a C-string conversion ends at the first NUL. A nonterminated character buffer must be passed with its length. Pointer **variables**, including character pointers and function pointers on the supported platform, hash their address without dereferencing. `nullptr` is supported. The adapter is non-owning: callers retain valid pointer lifetime, pointee mutation does not change the hash, and reuse of an address reuses its identity. This intentionally preserves the existing pointer/text overload distinction.

Compound strings/ranges/pairs/128-bit keys compress a larger domain into 64 bits and can collide. The former pair shift/xor combiner discarded high and low bits; its deterministic collision is now a regression test. The new recursive mixing avoids that particular loss but is neither universal hashing nor a cryptographic hash. There is **no proven `2^-64` collision probability** or adaptive-adversary resistance for this seeded family. A known seed permits collision construction; a clock-derived seed must not be treated as a secret. `safe_unordered_map`/`safe_unordered_set` retain exact key equality, so collisions affect performance, not logical answers: usual expected constant-time table operations can degrade to linear time per operation. The historical name `safe` does not change this guarantee.

## Feature-to-test map

| Feature | Verification |
|---|---|
| MT engine, explicit/default seeds, reset, copy/move, legacy global | Standard MT19937-64 10000th-word fixture, replay/state comparisons, default constructor smoke, global identity across translation units |
| Every bounded draw API and full signed/unsigned width | Arbitrary-precision Python mapping against raw engine words; minimum/maximum/singleton/full-width intervals, powers of two and neighbors, bounds/replay checks |
| Rejection uniformity argument | Exhaustive reduced words 1–10 bits for every nonzero width; full run exercises 22,335 rejected words in the exact oracle |
| Fisher–Yates and iterator requirements | Exact Python shuffle oracle, exhaustive abstract choices through n=7, empty/singleton/duplicates/deque/proxy/move-only cases, retained permutation and replay |
| Invalid Random preconditions | Five checked assertion probes: zero bound, reversed int/lng/ulng intervals and reversed shuffle range |
| SplitMix/scalar/128-bit/combine | Arbitrary-precision Python fixtures, boundary/random values, inverse-mixer round-trip, 128-bit high-word regression, scalar injectivity sample |
| Numeric conversion, enums, copies/seeds | Signed and unsigned extrema, scoped enum, bool, custom conversion, valid floats/signed zero, copy/move and fixed process seed |
| Pointer identity/lifetime | Pointee mutation, distinct addresses with equal values, typed/void/null/function pointers, pointer-key map and C-string pointer/array distinction |
| Strings | Lengths crossing every nearby 8-byte boundary, empty/binary/high-byte/NUL/unaligned buffers, Python byte-order oracle |
| Pair/tuple/ranges/aliases | Old guaranteed pair collision regression; empty/nested tuples; vector/list/forward-list/array/span/vector-bool; order/length fixtures; seeded map compared with ordered-map oracle; binary-string set and rehash |
| Header/linkage | Each tester includes only its actual canonical header; persistent optimized two-TU test in both link orders verifies early hash values, shared seed address and global Random address. Package integration separately owns aggregate compilation. |

The runnable Python entries accept `--mode quick/full/stress`, `--seed`, and an optional `--configuration`. Quick runs optimized/checked builds, 5,000 random regression draws, smaller hash dictionaries/oracle seeds and reduced-word enumeration through 8 bits. Full runs optimized, checked and ASan/UBSan builds; 100,000 C++ random/hash iterations, 750 exact Random oracle cases and 25,682 hash oracle cases **per configuration**, exhaustive reduced words through 10 bits, five Random assertion probes, and both optimized two-TU link orders. Stress increases C++ workloads to 1,000,000 Random / 500,000 hash iterations and the seeded oracle counts; it was not required or run for completion. Oracles use explicit failures that survive `-DNDEBUG`. Probabilistic fixed-seed frequency checks are regression checks, not proofs of uniformity.

## Commands and results

Executed with GCC 16.2.1 (`20260810`), GNU++20, Linux x86-64 on Intel Core i9-11900H, 2026-09-27:

```sh
python3 '96-Local Testing/06-Miscellaneous/01-random_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/06-Miscellaneous/02-customhash_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/06-Miscellaneous/01-random_tester.py' --mode full --seed 20260927 --configuration ASan-UBSan
python3 '96-Local Testing/06-Miscellaneous/02-customhash_tester.py' --mode full --seed 20260927 --configuration ASan-UBSan
```

Optimized (`-O2 -DNDEBUG`) and checked (`-O0 -g -D_GLIBCXX_DEBUG`) passed. The sandbox rejected LeakSanitizer's ptrace-based operation; the separate full ASan/UBSan configurations were rerun outside that sandbox and **passed**, with leak detection enabled. Sanitizer flags are `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie`. This is environment-specific rerun evidence, not a disabled leak check. No architecture-specific acceleration, reduction backend or threshold requires a performance benchmark here; no universal throughput claim is made. No online submission was performed. No remaining MI01 random/hash feature gap is known.

## Sources actually inspected

All accessed 2026-09-27. Implementations were written locally using the cited algorithm descriptions, while the preexisting SplitMix constants/finalizer were retained and checked against the public-domain source.

- Daniel Lemire, [*Fast Random Integer Generation in an Interval*, arXiv:1805.10941v4](https://arxiv.org/html/1805.10941v4): read the Fisher–Yates description/Algorithm 1 and section 4, Lemma 4.1 and Algorithm 5. The section provides the balanced-preimage argument and division-avoiding threshold computation; its experimental timing claims are not claimed as local benchmarks.
- [cppreference `std::mersenne_twister_engine`](https://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine.html): inspected the state/transition interface, `mt19937_64` parameters, default seed and reset/copy interface. The standardized engine is retained, rather than implementing another PRNG recurrence.
- Sebastiano Vigna, [*splitmix64.c*](https://prng.di.unimi.it/splitmix64.c), source marked 2015: read its full source and public-domain dedication/permissive license notice. The increment, odd multipliers and shift constants match the header. The file's BigCrush comment is source context, not a collision or cryptographic guarantee for these adapters.
- Local `97-Legacy/01-random.cpp` and `02-customhash.cpp`: read in full for legacy API accounting. `OLD` and these archived excerpts remain unchanged. The public random methods, engine, hash adapters, aliases and SplitMix helper remain; compound output values change to fix avoidable collisions and make explicit seeds useful.
