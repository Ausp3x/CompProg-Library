# 09-runlength.hpp — evidence

`09-runlength.hpp` (batch ST19, package P016) implements run-length encoding, statically and incrementally (`RunLengthEncoder`), over sequences of any `std::equality_comparable` and `std::copy_constructible` symbol type (both constraints are checked, since every operation copies symbols): bytes, integers, strings, pairs and other copyable values (user decision 2026-11-14, widened from integral symbols). Symbols are copied, never transformed; strings include embedded NUL and all byte values. The encoder accepts an iterable sequence with such a `value_type` and `size()`, including strings, string views and vectors. Source length and encoded run count are at most `INT_MAX`. Complexity symbols: `r` is the run count (`runs.size()`, or the number of maximal runs produced) and `n` the source or decoded length. Counts and decoded witness positions use `lng` because externally supplied encodings can describe up to `INT64_MAX` symbols without expansion.

## Contracts

### runLengthEncode, runLengthSize, runLengthDecode, runLengthSpans

| Operation | Contract and cost |
|---|---|
| `runLengthEncode(s,runs,max_count)` | Produce maximal adjacent runs as `(symbol,positive count)` pairs. Linear in input length, with output space proportional to the number of runs. Return false and clear output if a run exceeds `max_count`; the default is `INT64_MAX`. A zero cap accepts only empty input. |
| `runLengthSize(runs,size,max_size)` | Validate positive counts and compute the decoded size without expanding. Reject totals above the cap before signed overflow, return false and set `size=0`. Linear in run count and constant auxiliary space. The default cap is `INT64_MAX`. The output may alias an input count because assignment occurs after the final needed read. |
| `runLengthDecode(runs,s,max_size)` | Decode into a string or vector whose element type is the runs' symbol type. Validate every count, the requested cap, and output container `max_size()` before allocation. The default and largest accepted cap is `INT_MAX`; return false with empty output for malformed or oversized encodings. Time is linear in run count plus decoded length, with decoded output space. |
| `runLengthSpans(runs,spans,max_size)` | Return each input run's half-open decoded interval, including nonmaximal adjacent equal runs. Uses the same validation and default cap as `runLengthSize`. Linear in run count and output space; no decoded symbols are allocated. Temporary output permits input/output aliasing for `lng` symbols. `runLengthSize` and `runLengthSpans` never read the symbols, so they take runs of any symbol type. Failure clears output, including aliased input. |

All caps must be nonnegative; negative caps and source/run counts above the declared domain are precondition violations. Zero or negative encoded counts are handled invalid data, not precondition violations. All operations distinguish successful empty data from failure by their boolean return. Decoding and spans deliberately accept adjacent equal runs, while re-encoding produces maximal runs. Allocation exhaustion retains ordinary C++ allocation exceptions; a caller should choose a practical decode cap for its memory budget.

Symbols are equal exactly when `==` says so (user decision 2026-11-14). For floating-point symbols this means NaN never joins a run (each NaN is its own run) and `-0.0` joins a `0.0` run, which keeps the first symbol's sign, so decoding can change the sign of a zero; callers needing bitwise identity encode `std::bit_cast` integers instead.

### RunLengthEncoder

| Member | Contract and cost |
|---|---|
| `ctor ()`, `ctor (sequence)` | The default constructor makes an empty encoder; `RunLengthEncoder<T>(s)` (explicit, `s.size() <= INT_MAX`, `value_type` exactly `T`) encodes a whole sequence through `runLengthEncode` and sets `length() = s.size()`; O(n), output O(r). A deduction guide makes `RunLengthEncoder enc(s)` take `T = S::value_type`. The user-declared constructors make the struct a non-aggregate, so no brace form can set `runs` without `n` (user decision 2026-11-14). Assigning `{}` clears an encoder. |
| `runs` | The encoding of the current sequence as maximal runs with positive `lng` counts, so the free functions above read it directly. Callers read it only; writing it outside the members breaks the `length()` invariant. Space is O(r) for the most runs held so far, since `popBack` keeps the vector's capacity. The encoder does not bound its run count, while the free functions assert `runs.size() <= INT_MAX`; reaching that bound needs over 32 GB of runs. |
| `length()` | Decoded length, kept in `n`; O(1). Named as in the row and suisen, beside the free `runLengthSize`, which validates an external encoding and returns a status (user decision 2026-11-14). |
| `pushBack(x,count=1)` | Append `count` copies of `x` (taken by `const T &`, which may refer to a symbol in `runs` itself; the libstdc++ `emplace_back` copies it before reallocating): extend the last run when its symbol equals `x`, otherwise append a run; `count = 0` changes nothing and adds no run. Precondition `0 <= count <= INT64_MAX - length()`. O(1) amortized (vector growth). |
| `popBack(count=1)` | Remove the last `count` symbols, deleting each run it empties and shortening the last one it reaches. Precondition `0 <= count <= length()`, asserted and with no return value: a count past the length is a caller error that aborts at the bad call, where suisen's clamp would continue silently; a caller wanting the clamp writes `popBack(min(k, length()))` (user decision 2026-11-14). One call costs O(1 + runs removed); each run is removed at most once after the `pushBack` that created it, so a sequence of operations costs O(1) amortized each. |

Under `NDEBUG` a violated `popBack` precondition still terminates: the loop stops when `runs` is empty, leaving a meaningless `length()`.

Correctness and optimality: the encoder starts a run precisely at the first symbol or an adjacent-symbol change. Its count therefore equals the length of that maximal constant interval, and the concatenated intervals partition the source. Counts are tested against their cap before incrementing.

Size validation maintains a nonnegative partial sum `n <= max_size`. A positive count is accepted only if `count <= max_size-n`, so both the subtraction and subsequent addition are representable. Any rejected count or sum is detected before expansion. Decoding appends the validated number of copies of each symbol, and spans accumulate the same validated lengths. The span temporary preserves unread input under aliasing. `RunLengthEncoder` keeps the invariant that `runs` is the maximal encoding of the current sequence and `n` its length: `pushBack` with a positive count either extends the last run, whose symbol equals `x`, or appends a run whose symbol differs from the previous last one, so adjacent runs still differ; `popBack` removes `count` symbols from the back by taking `min(count, last count)` per step, deleting emptied runs, and a prefix of a maximal encoding with its last run shortened stays maximal. The sequence constructor holds the invariant because `runLengthEncode` returns the maximal encoding of `s` with total `s.size()`. These arguments depend only on symbol equality and copying, so they hold for every `std::equality_comparable` symbol type whose `==` is an equivalence; for floating-point `==` (not reflexive on NaN) they still describe the runs `==` defines.

Encoding must inspect all source symbols, size validation must inspect all runs, and decoding must write every output symbol. The respective linear input/output bounds are optimal for these explicit representations.

## Feature-to-test map

The entry point is `96-Local Testing/07-Strings/09-runlength_tester.py`.

| Feature | Verification |
|---|---|
| runLengthEncode: maximal encoding and count cap | Independent adjacent-change boundary oracle; all ternary sequences through length eight in full mode; exact cap and one below; empty cap zero; deterministic random sequences. |
| runLengthDecode: decode and roundtrip | Direct expansion of bounded external encodings, source roundtrips, exact decoded cap and one below, repeated calls and clearing old output. |
| Malformed/nonmaximal encodings | Exhaustive run lists through three entries with two symbols and counts `-1..3`, under every cap `0..10`; adjacent equal runs remain valid. |
| runLengthSize: size/count overflow | Independent signed-128-bit sum oracle, `INT64_MIN`, zero/negative counts, one `INT64_MAX` run, total exactly `INT64_MAX`, total `INT64_MAX+1`, caps one below, and deterministic random full-width counts. No oversized expansion is attempted. |
| Container capacity | A vector with an allocator reporting capacity three accepts decoded size three and rejects four before allocation. |
| runLengthSpans: witness spans and aliasing | Expected source-boundary spans, direct external-run positions, successful and failed input/output span aliases, and successful/failed output aliases into an input count for size queries. |
| Alphabet and size domains | Empty/singleton, full byte alphabet with embedded NUL/high bytes, string-view source, signed/unsigned integer extremes through 64 bits, `vector<bool>`, `vector<string>` and `vector<pair<int, int>>` fixtures plus every random source case mapped to strings and to pairs, floating symbols `0.0, -0.0, NaN, NaN, 1.5` (NaN runs separate, signed zero merged keeping `+0.0`, decoded sign), 200,000-symbol unary/alternating inputs, and a sequence reporting `size() = INT_MAX` (two stored symbols) that the encoder accepts. Other symbol types follow the same copy/equality-only algorithm but are not separate executed fixtures. |
| RunLengthEncoder: ctor, pushBack, popBack, length | Every sequence of pushBack (symbols 0/1, counts 0..2) and popBack (every count 1..length) through 4/6/7 operations in quick/full/stress, plus 300/4,000/20,000 random sequences of up to 39 operations over three symbols, each replayed again with string symbols; after each operation `length()` against a plain vector oracle and `runs` against the independent boundary oracle, then `runLengthDecode(runs)` against the vector, `RunLengthEncoder<T>(vector)` and the deduced `RunLengthEncoder(vector)` against the boundary oracle and the vector length, and `enc = {}` clearing it. Default arguments, `popBack(0)` and `pushBack(x, 0)` on an empty encoder, `popBack(0)` on a nonempty one, push after pop into a shortened run; `char`, `bool`, `unsigned char`, `int`, `lng`, `ulng`, `string` and `pair<int, int>` symbols, each also built from a three-symbol vector; deduced construction from a string with embedded NUL and explicit construction from a string view; `pushBack` of a symbol read from `runs` across a reallocation (`shrink_to_fit` first) and with a count read from `runs`; `INT64_MAX` length in one run and over three runs with bulk pops across runs; 10,000/200,000/1,000,000 unit pushes and pops and one bulk pop of the whole sequence. |
| Preconditions | Ten checked-build assertion probes: oversized source, negative encode cap, negative size cap, negative decode cap, negative span cap, negative push count, push past `INT64_MAX`, negative pop count, pop past the length and an encoder built from an oversized source. |

Quick/full/stress enumerate ternary strings through lengths 5/8/10, run 300/4,000/20,000 random source cases, enumerate external run lists through 2/3/4 entries, and exercise large inputs of 10,000/200,000/1,000,000 symbols; the encoder counts are in its map row. Every mode covers all public operations.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 880 MB; optimized 0.1 s compile + 0.6 s run, checked 22.3 s compile + 2.2 s run, ASan-UBSan 26.3 s compile + 8.0 s run; MEMORY peak 1029 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 927 MB; optimized 0.3 s compile + 0.6 s run, checked 29.3 s compile + 2.9 s run, ASan-UBSan 34.1 s compile + 7.6 s run; MEMORY peak 1051 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 33.3 s compile + 0.1 s run, checked 22.2 s compile + 0.8 s run; MEMORY peak 850 MB. Dated rows below stay those commands' latest results.

Run 2026-11-14 (ST19; experiment: none; Intel Core i9-11900H, GNU++20). Configurations: optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan, as built by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>). Every line passed:

```bash
python3 '96-Local Testing/07-Strings/09-runlength_tester.py' --mode full --seed 1  # PASS, 3 configurations, 5,623,366 checks each, 10 assertion probes; pre-flight quick peak 1037 MB, MEMORY peak 1362 MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/09-runlength_tester.py' --mode full --seed 2  # PASS, 3 configurations, 5,623,932 checks each, 10 assertion probes; pre-flight quick peak 1050 MB, MEMORY peak 1345 MB
python3 '96-Local Testing/07-Strings/09-runlength_tester.py' --mode stress --seed 3  # PASS, 3 configurations, 54,064,844 checks each, 10 assertion probes; optimized 10.1 s, checked 17.7 s, ASan-UBSan 124.8 s run; MEMORY peak 1042 MB
python3 '96-Local Testing/05-extraction.py' ops --filter '07-Strings/09-runlength.hpp'  # PASS, 8 operations (ctor included) on g++ and g++-14
python3 '96-Local Testing/05-extraction.py' testers --filter '09-runlength'  # PASS, 1 quick run on the extracted build
python3 '96-Local Testing/02-integration.py'  # PASS, 190 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace; MEMORY peak 1639 MB
python3 '96-Local Testing/03-consistency.py'  # no errors
```

The stress ASan-UBSan run is dominated by the 7-operation exhaustive encoder enumeration; it stays far below the 720 s limit.

## Mutation testing

Record: mewt 4.0.0 on blob `f03b04a75d07a29a2c60996a10980b70043e6e81` (2026-11-14, seed 1, 3 lines changed since blob `88b0a9ebf6a8`): 0 mutants, 0 caught (0 by timeout), 0 survived, 0 untested.

The 3 changed lines add `std::copy_constructible` to two constraints and the constructor's domain to a comment. Blob `88b0a9ebf6a8` (17 lines changed since blob `5296358e3e94`: symbol widening, the sequence constructor, the deleted decode assert, comments): 9 mutants, 9 caught, 0 survived, quick stage 18 s. Blob `5296358e3e94` changed only a comment line of blob `be1f566a94ca` (0 mutants). The whole-file campaign on blob `be1f566a94ca` (seed 1), whose line numbers the list below uses: 242 mutants, 233 caught (2 by timeout), 9 survived, 0 untested. Command `python3 '96-Local Testing/04-mutation.py' run '07-Strings/09-runlength.hpp'`; stage seconds 374 quick, 19 full-optimized, 288 full-rest, then the survivor retest with `--workdir` 390, 24, 376. The first run left `COS 154` (line 8, `s.size() < INT_MAX`), closed by the `ReportedSizeSequence` fixture reporting `size() = INT_MAX`. Its nine survivors were equivalent; `CR 50` left with its line, and the other eight lines are unchanged:

- `COS 171` (line 15, `==` to `>=`): with `max_count >= 0` a count is incremented only after this test fails, so it never exceeds `max_count` and the two comparisons agree; they differ only for a negative cap, a violated precondition, where both versions still end within the linear bound (user decision 2026-11-12).
- `COS 173`, `COS 174` (line 22, `runs.size() <= INT_MAX` to `!=` or `<`): they differ only at `runs.size() >= INT_MAX`, a vector of 32 GB that no memory-capped test can build; untestable under the INT_MAX size-assert decisions of 2026-11-12 and 2026-11-13.
- `CR 45` (line 35), `CR 54` (line 50): removing `reserve` changes only reallocation, not the contents or the amortized bound.
- `MR 238` (line 40): copying `res` instead of moving it assigns the same spans at the same O(r) cost.
- `CR 50` (line 46): the redundant `assert(max_size >= 0)` in `runLengthDecode`, deleted by `/finish` since `runLengthSize` asserts it on every path.
- `COS 228` (line 71, `count > 0` to `!= 0`): they differ only for a negative count, a violated precondition; under `NDEBUG` one iteration sets `count` to zero and the call ends (user decision 2026-11-12).
- `AS 93` (line 72): `min(count, back)` and `min(back, count)` return equal values.

## Benchmarks

No benchmark: a direct linear scan with no specialization, dispatch threshold or speed claim. Large unary and alternating cases exercise both extreme output sizes.

## Sources

Read on 2026-09-28; the first two sources were inspected in full and used for algorithm/contract comparison, not copied code:

- [maspypy, `string/run_length.hpp`](https://github.com/maspypy/library/blob/main/string/run_length.hpp): generic sequence encoding with signed 64-bit counts and empty-input handling. No decoder or overflow checks are provided by that reference.
- [Nyaan, `string/run-length-encoding.hpp`](https://github.com/NyaanNyaan/library/blob/master/string/run-length-encoding.hpp): maximal adjacent runs, explicit empty case, generic sequence element type and `int` counts. The maintained implementation adds explicit count/size contracts and independently designed decoding/span checks.

- [suisen, `util/run_length_encoder.hpp`](https://github.com/suisen-cp/cp-library-cpp/blob/main/library/util/run_length_encoder.hpp), read 2026-11-14: the incremental encoder (`push_back(val, num)`, `pop_back(num)` clamping to the length, `length()`). Its `pop_back` early-return path leaves `_length` unchanged and `clear`/`swap` skip `_length`; `RunLengthEncoder` keeps one length update per call and asserts instead of clamping. Name map: `push_back` is `pushBack`, `pop_back` is `popBack`.

Catalog sweep 2026-10-08 (Nyaan, maspypy, suisen `util/run_length_encoder.hpp`, hitonanode, ei1333, tko919, KACTL, PyRival) in [00-sources.md](00-sources.md): no further adopted operation. ST19 re-sweep 2026-11-14 (suisen, Nyaan, maspypy, hitonanode, emthrm): only suisen has incremental operations, now `RunLengthEncoder`; no front-side variant and no slower alternative in any catalog.

Targeted path/symbol searches in `OLD/algorithms.cpp`, `OLD/[1] algorithms.cpp`, and Team Notebook `src/algs.cpp`/`src/algsbetter.cpp` found no run-length implementation to migrate.

## Limits and handoffs

- suisen's other `RunLengthEncoder` members are other forms of these operations (reason in [00-notes.md](00-notes.md)): `front`/`back`/iterators read `runs`, `swap` and `clear` are assignment; its container constructor is `ctor (sequence)`.
- Miscellaneous retains Huffman coding; Lempel–Ziv (`42`) and run-length BWT (`46`) remain separate Strings owners; streaming merge state is not part of these static contracts.

## Aliases

Search aliases (P263, grab --find): 游程编码, 行程编码, 行程长度编码.

## History

- 2026-09-28: verified under the previous system (P016).
- 2026-10-08: re-audit baseline, full suite seed 1 passed in 3 configurations before any change.
- 2026-10-08: re-audit, 2 findings fixed, contracts moved out of the header, behavior unchanged; quick, full and stress passed on g++, full on g++-14, integration with sanitizers passed.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (`grab --find`; Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-14: ST19 added `RunLengthEncoder` (pushBack, popBack, length), returned into the row by P275; re-sweep found nothing missing; whole-file mutation campaign closed one gap (source size exactly `INT_MAX`), 9 equivalent survivors; review fixed 1 medium (space bound after `popBack`) and 4 low findings; full on g++ and g++-14, stress, extraction and integration passed.
- 2026-11-14: `/finish P016`: audit found 2 medium and 3 low findings, all fixed; user decisions: `ctor (sequence)` with a deduction guide, `popBack` keeps its assert, the whole row widened to `std::equality_comparable` symbols, `length()` keeps its name; the diff review's 3 low findings fixed (copy constraint, constructor domain comment, aliasing test); changed-line mutation campaigns caught 9 of 9; full on g++ and g++-14, stress, extraction and integration passed.
