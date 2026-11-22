# 09-runlength.hpp — Evidence

`09-runlength.hpp` (batch ST19, package P016) implements run-length encoding, statically and incrementally (`RunLengthEncoder`), over sequences of any `std::equality_comparable` and `std::copy_constructible` symbol type: bytes, integers, strings, pairs and other copyable values (user decision 2026-11-14, widened from integral symbols). Both constraints are checked, since every operation copies symbols. Symbols are copied, never transformed; strings include embedded NUL and every byte. The encoder accepts any iterable sequence with such a `value_type` and `size()`. Source length and run count are at most `INT_MAX`. In the costs, `r` is the run count (`runs.size()`, or the maximal runs produced) and `n` the source or decoded length. Counts and decoded positions are `lng`, because an external encoding can describe up to `INT64_MAX` symbols without expansion.

## Contracts

### runLengthEncode, runLengthSize, runLengthDecode, runLengthSpans

| Operation | Contract and cost |
|---|---|
| `runLengthEncode(s,runs,max_count)` | Produces maximal adjacent runs as `(symbol, positive count)` pairs in O(n) time and O(r) output. Returns false and clears the output if a run exceeds `max_count` (default `INT64_MAX`); a zero cap accepts only empty input. |
| `runLengthSize(runs,size,max_size)` | Validates positive counts and computes the decoded size without expanding, in O(r) time and O(1) extra space. A total above the cap (default `INT64_MAX`) is rejected before signed overflow: returns false with `size = 0`. The output may alias an input count, since it is assigned after the last read. |
| `runLengthDecode(runs,s,max_size)` | Decodes into a string or vector whose element type is the runs' symbol type, in O(r + n) time and O(n) output. Checks every count, the cap and the container's `max_size()` before allocating. The default and largest cap is `INT_MAX`; a malformed or oversized encoding returns false with empty output. |
| `runLengthSpans(runs,spans,max_size)` | Returns each input run's half-open decoded interval, nonmaximal adjacent equal runs included, with `runLengthSize`'s validation and default cap; O(r) time and output, no decoded symbols. A temporary output permits input/output aliasing for `lng` symbols. Failure clears the output, aliased input included. |

`runLengthSize` and `runLengthSpans` never read symbols, so they take runs of any symbol type. Caps must be nonnegative; a negative cap or a source or run count above the domain is a precondition violation, while a zero or negative encoded count is invalid data the functions reject. The boolean return separates a successful empty result from failure. Decoding and spans accept adjacent equal runs; re-encoding produces maximal runs. Allocation failure throws as usual in C++, so a caller picks a decode cap within its memory budget.

Symbols are equal exactly when `==` says so (user decision 2026-11-14). For floating-point symbols, NaN never joins a run, and `-0.0` joins a `0.0` run, which keeps the first symbol's sign, so decoding can change a zero's sign; callers needing bitwise identity encode `std::bit_cast` integers.

### RunLengthEncoder

| Member | Contract and cost |
|---|---|
| `ctor ()`, `ctor (sequence)` | The default constructor makes an empty encoder. `RunLengthEncoder<T>(s)` (explicit, `s.size() <= INT_MAX`, `value_type` exactly `T`) encodes `s` through `runLengthEncode` and sets `length() = s.size()`; O(n), output O(r). A deduction guide makes `RunLengthEncoder enc(s)` take `T = S::value_type`. The user-declared constructors make the struct a non-aggregate, so no brace form sets `runs` without `n` (user decision 2026-11-14). Assigning `{}` clears an encoder. |
| `runs` | The maximal encoding of the current sequence with positive `lng` counts, so the free functions read it directly. Callers only read it; writing it breaks the `length()` invariant. Space is O(r) for the most runs held so far, since `popBack` keeps capacity. The encoder does not bound its run count, while the free functions assert `runs.size() <= INT_MAX`, which needs over 32 GB of runs. |
| `length()` | Decoded length, kept in `n`; O(1). Named as in the row and suisen, beside the free `runLengthSize`, which validates an external encoding and returns a status (user decision 2026-11-14). |
| `pushBack(x,count=1)` | Appends `count` copies of `x`: extends the last run when its symbol equals `x`, otherwise appends a run; `count = 0` changes nothing. `x` is taken by `const T &` and may refer into `runs`, since libstdc++ `emplace_back` copies it before reallocating. Precondition `0 <= count <= INT64_MAX - length()`. O(1) amortized. |
| `popBack(count=1)` | Removes the last `count` symbols, deleting each run it empties and shortening the last one it reaches. Precondition `0 <= count <= length()`, asserted, no return value: a count past the length aborts at the bad call, where suisen's clamp would continue silently; a caller wanting the clamp writes `popBack(min(k, length()))` (user decision 2026-11-14). One call costs O(1 + runs removed), O(1) amortized, since each run is removed at most once after the `pushBack` that created it. |

Under `NDEBUG` a violated `popBack` precondition still terminates: the loop stops when `runs` is empty, leaving a meaningless `length()`.

Correctness: the encoder starts a run exactly at the first symbol or at a change between adjacent symbols, so each count is the length of a maximal constant interval and the intervals partition the source; counts are tested against the cap before incrementing. Size validation keeps a partial sum `0 <= n <= max_size` and accepts a positive count only if `count <= max_size - n`, so the subtraction and the addition are representable and every rejection precedes expansion. Decoding appends the validated copies and spans accumulate the same lengths; the span temporary preserves unread input under aliasing. `RunLengthEncoder` keeps `runs` the maximal encoding of the current sequence and `n` its length: a positive `pushBack` extends a last run equal to `x` or appends a differing run; `popBack` takes `min(count, last count)` per step and deletes emptied runs, and a prefix of a maximal encoding with its last run shortened stays maximal; the sequence constructor starts from `runLengthEncode`'s maximal encoding with total `s.size()`. These arguments use only equality and copying, so they hold whenever `==` is an equivalence; for floating-point `==` they describe the runs `==` defines.

Encoding must read every symbol, size validation every run and decoding must write every output symbol, so the linear bounds are optimal.

## Feature-to-Test Map

Runner: `96-Local Testing/07-Strings/09-runlength_tester.py`. Quick, full and stress enumerate ternary strings through lengths 5/8/10, run 300/4,000/20,000 random source cases, enumerate external run lists through 2/3/4 entries and run large inputs of 10,000/200,000/1,000,000 symbols. Every mode covers every operation.

| Feature | Verification |
|---|---|
| runLengthEncode: maximal encoding and count cap | Independent adjacent-change boundary oracle; all ternary sequences through length eight in full mode; exact cap and one below; zero cap on empty input; seeded random sequences. |
| runLengthDecode: decode and roundtrip | Direct expansion of bounded external encodings, source roundtrips, exact decoded cap and one below, repeated calls clearing old output. |
| Malformed and nonmaximal encodings | Every run list through three entries over two symbols with counts `-1..3`, under every cap `0..10`; adjacent equal runs stay valid. |
| runLengthSize: size and count overflow | Independent signed 128-bit sum oracle; `INT64_MIN`, zero and negative counts, one `INT64_MAX` run, totals `INT64_MAX` and `INT64_MAX + 1`, caps one below, seeded random full-width counts. No oversized expansion is attempted. |
| Container capacity | A vector whose allocator reports capacity three accepts decoded size three and rejects four before allocating. |
| runLengthSpans: spans and aliasing | Expected source-boundary spans, direct external-run positions, successful and failed input/output span aliases, and successful and failed output aliases into an input count for size queries. |
| Alphabet and size domains | Empty and singleton inputs; the full byte alphabet with embedded NUL and high bytes; a string-view source; signed and unsigned integer extremes through 64 bits; `vector<bool>`, `vector<string>` and `vector<pair<int, int>>` fixtures, and every random source case mapped to strings and to pairs; floating symbols `0.0, -0.0, NaN, NaN, 1.5` (separate NaN runs, signed zero merged keeping `+0.0`, decoded sign); 200,000-symbol unary and alternating inputs; a `ReportedSizeSequence` reporting `size() = INT_MAX` with two stored symbols, which the encoder accepts. Other symbol types run the same copy-and-equality algorithm without separate fixtures. |
| RunLengthEncoder: ctor, pushBack, popBack, length | Every sequence of `pushBack` (symbols 0/1, counts 0..2) and `popBack` (every count 1..length) through 4/6/7 operations in quick/full/stress, plus 300/4,000/20,000 random sequences of up to 39 operations over three symbols, replayed with string symbols. After each operation `length()` is checked against a vector oracle, `runs` against the boundary oracle, `runLengthDecode(runs)` against the vector, `RunLengthEncoder<T>(vector)` and the deduced `RunLengthEncoder(vector)` against the oracle and length, and `enc = {}` clears. Also: default arguments; `popBack(0)` and `pushBack(x, 0)` on an empty encoder and `popBack(0)` on a nonempty one; a push after a pop into a shortened run; `char`, `bool`, `unsigned char`, `int`, `lng`, `ulng`, `string` and `pair<int, int>` symbols, each also built from a three-symbol vector; deduced construction from a string with embedded NUL and explicit construction from a string view; `pushBack` of a symbol read from `runs` across a reallocation (`shrink_to_fit` first) and with a count read from `runs`; `INT64_MAX` length in one run and over three runs with bulk pops across runs; 10,000/200,000/1,000,000 unit pushes and pops and one bulk pop of the whole sequence. |
| Preconditions | Ten checked-build assertion probes: oversized source, negative encode, size, decode and span caps, negative push count, push past `INT64_MAX`, negative pop count, pop past the length, and an encoder built from an oversized source. |

## Commands and Results

Configurations: optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan-UBSan, as built by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>), on an Intel Core i9-11900H with GNU++20.

Library-wide runs 2026-11-10 (P274; seed 42, beside the other suites, build cache emptied before each cold run), this suite's results:

| Command | Result |
|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | PASS. Compile + run: optimized 0.1 + 0.6 s, checked 22.3 + 2.2 s, ASan-UBSan 26.3 + 8.0 s. Peak 1029 MB, pre-flight quick 880 MB. |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | PASS. Optimized 0.3 + 0.6 s, checked 29.3 + 2.9 s, ASan-UBSan 34.1 + 7.6 s. Peak 1051 MB, pre-flight quick 927 MB. |
| `python3 '96-Local Testing/01-run.py' --mode quick` | PASS. Optimized 33.3 + 0.1 s, checked 22.2 + 0.8 s. Peak 850 MB. |

Tester runs 2026-11-14 (ST19), the latest with check counts:

```bash
python3 '96-Local Testing/07-Strings/09-runlength_tester.py' --mode full --seed 1  # PASS, 3 configurations, 5,623,366 checks each, 10 assertion probes; pre-flight quick peak 1037 MB, MEMORY peak 1362 MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/09-runlength_tester.py' --mode full --seed 2  # PASS, 3 configurations, 5,623,932 checks each, 10 assertion probes; pre-flight quick peak 1050 MB, MEMORY peak 1345 MB
python3 '96-Local Testing/07-Strings/09-runlength_tester.py' --mode stress --seed 3  # PASS, 3 configurations, 54,064,844 checks each, 10 assertion probes; optimized 10.1 s, checked 17.7 s, ASan-UBSan 124.8 s run; MEMORY peak 1042 MB
python3 '96-Local Testing/05-extraction.py' ops --filter '07-Strings/09-runlength.hpp'  # PASS, 8 operations (ctor included) on g++ and g++-14
python3 '96-Local Testing/05-extraction.py' testers --filter '09-runlength'  # PASS, 1 quick run on the extracted build
python3 '96-Local Testing/02-integration.py'  # PASS, 190 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace; MEMORY peak 1639 MB
python3 '96-Local Testing/03-consistency.py'  # no errors
```

The stress ASan-UBSan time comes mostly from the 7-operation exhaustive encoder enumeration, far below the 720 s limit.

2026-11-14 `/finish P016`: full on g++ and g++-14, stress, extraction and integration passed (counts not recorded).

## Mutation Testing

Record: mewt 4.0.0 on blob `f03b04a75d07a29a2c60996a10980b70043e6e81` (2026-11-14, seed 1, 3 lines changed since blob `88b0a9ebf6a8`): 0 mutants, 0 caught (0 by timeout), 0 survived, 0 untested.

```bash
python3 '96-Local Testing/04-mutation.py' run '07-Strings/09-runlength.hpp'
```

Earlier campaigns: a whole-file campaign on 2026-11-14 (seed 1; 242 mutants, 233 caught, 2 of them by timeout, 9 survived) and a changed-line campaign of the `/finish` fixes (9 mutants, 9 caught). The survivors below come from the whole-file campaign; their lines are unchanged since, so their arguments still stand.

| Mutant | Line | Argument |
|---|---|---|
| `COS 171` (`==` → `>=`) | 15 | Equivalent: with `max_count >= 0` a count is incremented only after this test fails, so it never exceeds `max_count`; the two differ only for a negative cap, a violated precondition, and both still end within the linear bound (user decision 2026-11-12). |
| `COS 173`, `COS 174` (`runs.size() <= INT_MAX` → `!=`, `<`) | 22 | Untestable under the `INT_MAX` size-assert decisions of 2026-11-12 and 2026-11-13: they differ only at `runs.size() >= INT_MAX`, a 32 GB vector no memory-capped test can build. |
| `CR 45`, `CR 54` (`reserve` removed) | 36, 50 | Equivalent: only reallocation changes, not the contents or the amortized bound. |
| `MR 238` (copy instead of move) | 41 | Equivalent: the same spans are assigned at the same O(r) cost. |
| `COS 228` (`count > 0` → `!= 0`) | 74 | Equivalent: they differ only for a negative count, a violated precondition; under `NDEBUG` one iteration sets `count` to zero and the call ends (user decision 2026-11-12). |
| `AS 93` (`min` operands swapped) | 75 | Equivalent: `min` commutes. |

## Benchmarks

No benchmark: a direct linear scan with no specialization, dispatch threshold or speed claim. Large unary and alternating cases exercise both extreme output sizes.

## Sources

Code is independently written; the sources were compared for algorithm and contract, not copied.

- [maspypy, `string/run_length.hpp`](https://github.com/maspypy/library/blob/main/string/run_length.hpp) (read in full 2026-09-28): generic sequence encoding with signed 64-bit counts and empty-input handling; no decoder or overflow checks.
- [Nyaan, `string/run-length-encoding.hpp`](https://github.com/NyaanNyaan/library/blob/master/string/run-length-encoding.hpp) (read in full 2026-09-28): maximal adjacent runs, explicit empty case, generic element type and `int` counts. This header adds explicit count and size contracts and its own decoding and span checks.
- [suisen, `util/run_length_encoder.hpp`](https://github.com/suisen-cp/cp-library-cpp/blob/main/library/util/run_length_encoder.hpp) (read 2026-11-14): the incremental encoder (`push_back(val, num)`, `pop_back(num)` clamping to the length, `length()`). Its `pop_back` early return leaves `_length` unchanged and `clear`/`swap` skip `_length`; `RunLengthEncoder` updates the length once per call and asserts instead of clamping. `push_back` is `pushBack`, `pop_back` is `popBack`.
- Catalog sweep 2026-10-08 (Nyaan, maspypy, suisen `util/run_length_encoder.hpp`, hitonanode, ei1333, tko919, KACTL, PyRival) in [00-sources.md](00-sources.md): no further operation. ST19 re-sweep 2026-11-14 (suisen, Nyaan, maspypy, hitonanode, emthrm): only suisen has incremental operations, now `RunLengthEncoder`; no catalog has a front-side variant or a slower alternative.
- Targeted searches of `OLD/algorithms.cpp`, `OLD/[1] algorithms.cpp` and Team Notebook `src/algs.cpp`/`src/algsbetter.cpp` found no run-length code to migrate.

## Limits and Handoffs

- suisen's other `RunLengthEncoder` members are other forms of these operations (reason in [00-notes.md](00-notes.md)): `front`, `back` and iterators read `runs`, `swap` and `clear` are assignment, and its container constructor is `ctor (sequence)`.
- Miscellaneous keeps Huffman coding; Lempel–Ziv (`42`) and run-length BWT (`46`) are separate Strings rows; streaming merge state is not part of these static contracts.

## Aliases

Search aliases (P263, grab --find): 游程编码, 行程编码, 行程长度编码.

## History

- 2026-09-28: verified under the previous system (P016).
- 2026-10-08: re-audit baseline, full suite seed 1 passed in 3 configurations before any change.
- 2026-10-08: re-audit, 2 findings fixed, contracts moved out of the header, behavior unchanged; quick, full and stress passed on g++, full on g++-14, integration with sanitizers passed.
- 2026-11-07: P263 added Chinese search aliases (`grab --find`): common names of the row's operations, Simplified plus Taiwan or Hong Kong terms where they differ; no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-14: ST19 added `RunLengthEncoder` (pushBack, popBack, length), returned into the row by P275; re-sweep found nothing missing; whole-file mutation campaign closed one gap (source size exactly `INT_MAX`), 9 equivalent survivors; review fixed 1 medium (space bound after `popBack`) and 4 low findings; full on g++ and g++-14, stress, extraction and integration passed.
- 2026-11-14: `/finish P016`: audit found 2 medium and 3 low findings, all fixed; user decisions: `ctor (sequence)` with a deduction guide, `popBack` keeps its assert, the whole row widened to `std::equality_comparable` symbols, `length()` keeps its name; the diff review's 3 low findings fixed (copy constraint, constructor domain comment, aliasing test); changed-line mutation campaigns caught 9 of 9; full on g++ and g++-14, stress, extraction and integration passed.
