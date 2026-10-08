# 09-runlength.hpp — evidence

`09-runlength.hpp` (batch ST19, package P016) implements run-length encoding over byte and integer sequences. Symbols retain their exact type and value; strings include embedded NUL and all byte values. The encoder accepts an iterable sequence with integral `value_type` and `size()`, including strings, string views and vectors. Source length and encoded run count are at most `INT_MAX`. Complexity symbols: `r` is the run count (`runs.size()`, or the number of maximal runs produced) and `n` the source or decoded length. Counts and decoded witness positions use `lng` because externally supplied encodings can describe up to `INT64_MAX` symbols without expansion.

## Contracts

### runLengthEncode, runLengthSize, runLengthDecode, runLengthSpans

| Operation | Contract and cost |
|---|---|
| `runLengthEncode(s,runs,max_count)` | Produce maximal adjacent runs as `(symbol,positive count)` pairs. Linear in input length, with output space proportional to the number of runs. Return false and clear output if a run exceeds `max_count`; the default is `INT64_MAX`. A zero cap accepts only empty input. |
| `runLengthSize(runs,size,max_size)` | Validate positive counts and compute the decoded size without expanding. Reject totals above the cap before signed overflow, return false and set `size=0`. Linear in run count and constant auxiliary space. The default cap is `INT64_MAX`. The output may alias an input count because assignment occurs after the final needed read. |
| `runLengthDecode(runs,s,max_size)` | Decode into a string or vector with the matching integral element type. Validate every count, the requested cap, and output container `max_size()` before allocation. The default and largest accepted cap is `INT_MAX`; return false with empty output for malformed or oversized encodings. Time is linear in run count plus decoded length, with decoded output space. |
| `runLengthSpans(runs,spans,max_size)` | Return each input run's half-open decoded interval, including nonmaximal adjacent equal runs. Uses the same validation and default cap as `runLengthSize`. Linear in run count and output space; no decoded symbols are allocated. Temporary output permits input/output aliasing for `lng` symbols. Failure clears output, including aliased input. |

All caps must be nonnegative; negative caps and source/run counts above the declared domain are precondition violations. Zero or negative encoded counts are handled invalid data, not precondition violations. All operations distinguish successful empty data from failure by their boolean return. Decoding and spans deliberately accept adjacent equal runs, while re-encoding produces maximal runs. Allocation exhaustion retains ordinary C++ allocation exceptions; a caller should choose a practical decode cap for its memory budget.

Correctness and optimality: the encoder starts a run precisely at the first symbol or an adjacent-symbol change. Its count therefore equals the length of that maximal constant interval, and the concatenated intervals partition the source. Counts are tested against their cap before incrementing.

Size validation maintains a nonnegative partial sum `n <= max_size`. A positive count is accepted only if `count <= max_size-n`, so both the subtraction and subsequent addition are representable. Any rejected count or sum is detected before expansion. Decoding appends the validated number of copies of each symbol, and spans accumulate the same validated lengths. The span temporary preserves unread input under aliasing. These arguments depend only on symbol equality and copying, not the magnitude or signedness of symbols.

Encoding must inspect all source symbols, size validation must inspect all runs, and decoding must write every output symbol. The respective linear input/output bounds are optimal for these explicit representations.

## Feature-to-test map

The entry point is `96-Local Testing/07-Strings/09-runlength_tester.py`, using the shared Strings runner and the actual public header. Failure checks remain active under `-DNDEBUG` and report seed, mode, operation, input, expected and actual output; the driver adds compiler configuration and failing command.

| Feature | Verification |
|---|---|
| Maximal encoding and count cap | Independent adjacent-change boundary oracle; all ternary sequences through length eight in full mode; exact cap and one below; empty cap zero; deterministic random sequences. |
| Decode and roundtrip | Direct expansion of bounded external encodings, source roundtrips, exact decoded cap and one below, repeated calls and clearing old output. |
| Malformed/nonmaximal encodings | Exhaustive run lists through three entries with two symbols and counts `-1..3`, under every cap `0..10`; adjacent equal runs remain valid. |
| Size/count overflow | Independent signed-128-bit sum oracle, `INT64_MIN`, zero/negative counts, one `INT64_MAX` run, total exactly `INT64_MAX`, total `INT64_MAX+1`, caps one below, and deterministic random full-width counts. No oversized expansion is attempted. |
| Container capacity | A vector with an allocator reporting capacity three accepts decoded size three and rejects four before allocation. |
| Witness spans and aliasing | Expected source-boundary spans, direct external-run positions, successful and failed input/output span aliases, and successful/failed output aliases into an input count for size queries. |
| Alphabet and size domains | Empty/singleton, full byte alphabet with embedded NUL/high bytes, string-view source, signed/unsigned integer extremes through 64 bits, `vector<bool>`, and 200,000-symbol unary/alternating inputs. Larger GNU integral symbol types follow the same copy/equality-only algorithm but are not separate executed fixtures. |
| Preconditions | Five checked-build assertion probes: oversized source, negative encode cap, negative size cap, negative decode cap and negative span cap. |

Quick/full/stress enumerate ternary strings through lengths 5/8/10, run 300/4,000/20,000 random source cases, enumerate external run lists through 2/3/4 entries, and exercise large inputs of 10,000/200,000/1,000,000 symbols. Every mode covers all public operations; full and stress add sanitizers.

## Commands and results

Run 2026-10-08 with the configurations listed in [01-prefixfunction.md](01-prefixfunction.md#commands-and-results).

```bash
python3 '96-Local Testing/07-Strings/09-runlength_tester.py' --mode quick --seed 1  # PASS, 2 configurations
python3 '96-Local Testing/07-Strings/09-runlength_tester.py' --mode full --seed 1  # PASS, 3 configurations, 363,468 checks each, 5 assertion probes
CXX=g++-14 python3 '96-Local Testing/07-Strings/09-runlength_tester.py' --mode full --seed 2  # PASS, 3 configurations, 363,430 checks each (`CXX=g++-14`)
python3 '96-Local Testing/07-Strings/09-runlength_tester.py' --mode stress --seed 3  # PASS, 3 configurations, 3,057,370 checks each
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 102 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizers
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 1 --no-integration  # PASS, 9 suites
python3 '96-Local Testing/03-consistency.py'  # no errors
```

The suite was also run in full mode before any change (seed 1, all three configurations PASS) as the re-audit baseline.

## Benchmarks

No benchmark: a direct linear scan with no specialization, dispatch threshold or
speed claim. Large unary and alternating cases exercise both extreme output sizes.

## Sources

Read on 2026-09-28; both sources were inspected in full and used for algorithm/contract comparison, not copied code:

- [maspypy, `string/run_length.hpp`](https://github.com/maspypy/library/blob/main/string/run_length.hpp): generic sequence encoding with signed 64-bit counts and empty-input handling. No decoder or overflow checks are provided by that reference.
- [Nyaan, `string/run-length-encoding.hpp`](https://github.com/NyaanNyaan/library/blob/master/string/run-length-encoding.hpp): maximal adjacent runs, explicit empty case, generic sequence element type and `int` counts. The maintained implementation adds explicit count/size contracts and independently designed decoding/span checks.

Catalog sweep 2026-10-08 (Nyaan, maspypy, suisen `util/run_length_encoder.hpp`, hitonanode, ei1333, tko919, KACTL, PyRival) in [00-sources.md](00-sources.md): no further adopted operation.

Targeted path/symbol searches in `OLD/algorithms.cpp`, `OLD/[1] algorithms.cpp`, and Team Notebook `src/algs.cpp`/`src/algsbetter.cpp` found no run-length implementation to migrate.

## Limits and handoffs

- Not adopted (reason in [00-notes.md](00-notes.md)): suisen-style `pushBack`/`popBack` on an encoding (single unverified source; two-line caller idioms on the run vector).
- Miscellaneous retains Huffman coding; Lempel–Ziv (`42`) and run-length BWT (`46`) remain separate Strings owners; streaming merge state is not part of these static contracts.
- Exact GCC 14.2 and the Windows build were not run.

## History

- 2026-09-28: verified under the previous system (P016). 2026-10-08 re-audit: `/reaudit-review` findings 15 (closing braces) and 16 (undefined complexity symbols) fixed; `runLengthSize` and `runLengthSpans` share one complexity line; contracts moved out of the header. Behavior unchanged.
