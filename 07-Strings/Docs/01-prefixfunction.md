# 01-prefixfunction.hpp — evidence

`01-prefixfunction.hpp` (batch ST01, package P016) provides the prefix function, streaming/overlapping/empty-pattern KMP, prefix counts, borders/periods and the dense alphabet automaton, plus exact prefix-function validation. With `02-z.hpp` it implements all ST01 inventory features. They use exact symbol
equality, accept empty sequences, and reserve no separator value. `string` and
`string_view` include embedded NUL and every byte; integer vectors may use the
full range of their element type. String positions count bytes, not Unicode
code points. Generic sequence equality must be a consistent equivalence
relation with constant-time indexing/comparison. Every sequence/array length
is strictly less than `INT_MAX`; returned indices and lengths are `int`.

## Contracts

### prefixFunction, KmpMatcher, kmpOccurrences, prefixOccurrences, prefixBorders, prefixPeriod, prefixPeriods, prefixAutomaton, validPrefixFunction

| API | Result and cost |
|---|---|
| `prefixFunction(s)` | Longest proper border length for every prefix; O(n) time and returned storage. Empty returns `[]`. |
| `KmpMatcher<T>(pattern)` | Owns an O(m) copy and its prefix array. `step(symbol)` returns whether a match ends after that symbol; `state` retains the longest matching suffix length, including accepting state m. O(m) setup; O(1) amortized per step, O(m) worst case for one step. |
| `matched()`, `reset()` | O(1) current acceptance and reset of `state`/`processed`, retaining the owned pattern. `processed` is an exact `lng` count, limited to `INT64_MAX` between resets. |
| `kmpOccurrences(pattern,text)` | All overlapping start positions in ascending order; O(n+m) time, O(m+k) space for k answers. |
| `prefixOccurrences(pi)` | Exact `lng` counts of every original-string prefix, including `answer[0]=n+1`; O(n) time/space. |
| `prefixOccurrences(pattern,text)` | Exact `lng` counts of every pattern prefix in text, including `answer[0]=n+1`; O(n+m) time, O(m) space. |
| `prefixBorders(pi,len=-1,include_full=false)` | Ascending positive borders of prefix length `len`; -1 selects the whole string. Proper borders by default, optional full length. O(k) time/output space. |
| `prefixPeriod(pi,whole=false)` | Smallest positive shift period in O(1); `whole=true` returns the primitive-root length that divides n. Empty returns 0. |
| `prefixPeriods(pi)` | All positive shift periods, including n, ascending in O(k) time/output space. Empty returns `[]`. |
| `prefixAutomaton(pattern,sigma)` | Encoded symbols in `[0,sigma)` and sigma>=0. Rows 0..m include acceptance; `aut[q][c]` is the next suffix-match state. O(m+(m+1)*sigma) construction, O((m+1)*(sigma+1)) stored space, including row metadata even for sigma=0. Lookup O(1). |
| `validPrefixFunction(pi)`, `validZFunction(z)` | Exact feasibility over an unrestricted integer alphabet; O(n) time/space. Empty arrays are valid. These impose no fixed-alphabet or minimum-alphabet guarantee. |

Prefix/Z border, count and period helpers assume valid representations; call the
validators first for untrusted arrays. They do not spend O(n) revalidating each
output-sensitive border query. The dense automaton asserts every encoded
symbol's range; empty pattern with zero alphabet is valid and yields one empty
row. To encode bytes, convert through `unsigned char` before using 0..255.

An empty pattern matches all n+1 text boundaries. A new empty matcher already
has `matched()==true` at boundary zero; each `step` reports the next boundary.
When streaming chunks, emit the initial boundary once and retain the matcher
between chunks. Static occurrence helpers emit all n+1 boundaries directly.
The stream never stores text and can process more than `INT_MAX` symbols,
subject to the `lng` counter limit. The template `step` compares the original
symbol type directly, so a wider text symbol is not narrowed to the pattern
type. Constructors require iterable sequences; static prefix/Z functions only
require `.size()` and indexing. Matcher template argument deduction uses the
sequence's `value_type`; raw C arrays/string literals should be wrapped in a
`string_view`, `string` or vector.

Copies retain independent pattern and matching state. A moved-to matcher
retains that state; reassign a moved-from matcher before using it. Public
pattern/pi/state fields are contest-style implementation state and must not be
modified independently. There are no global caches or shared alphabet state.

## Correctness and complexity

The prefix failure chain enumerates every shorter candidate border in decreasing
length. Failed comparisons descend that chain; successful comparisons increase
the length by at most one. This proves the selected longest border and the
linear total comparison count. Streaming KMP uses the same invariant for the
already consumed text suffix; acceptance falls back at the beginning of the
next step, preserving overlapping matches. Each prefix is a node whose parent
is its longest proper border. Descending propagation of ending counts along
these edges gives every prefix count exactly once per occurrence. The
automaton reuses previously constructed parent rows, including after acceptance.

For prefix validation, a positive entry forces the endpoint equality
`s[i]=s[pi[i]-1]`; a zero entry receives a fresh integer symbol. Every equality
required by a feasible prefix array is generated by these endpoint equalities
(induct along the border's positions). Splitting otherwise unconstrained
classes cannot destroy a required border. Recomputing the prefix function
therefore accepts exactly feasible arrays and detects contradictory extra
or missing borders. This internal witness is not a fixed/minimum-alphabet or
lexicographically minimum reconstruction interface; ST25 owns those contracts.

## Feature-to-test map

Both mirrored tester entries import the Strings shared runner and execute the
actual headers. Oracles use direct substring comparisons/LCP scans and all
restricted-growth alphabet partitions, independent of failure chains and Z
boxes. Checks call explicit failure reporting that remains active with
`-DNDEBUG`.

| Feature | Coverage |
|---|---|
| Prefix/Z arrays, borders and periods | Exhaustive binary strings, direct definitions, every prefix border query, proper/full borders, divisor-restricted roots, empty/singleton/unary/periodic/random data. |
| KMP streaming/static matching and prefix counts | All bounded pattern/text pairs against naive occurrences; accepting state, overlaps, initial empty boundary, reset/replay, copied/moved state and exact processed count. |
| Dense automaton | Every state/symbol transition checked against a direct suffix scan, including acceptance, absent symbols and zero alphabet. |
| Validation/conversion | Enumerate every alphabet equality partition to derive the full feasible array set, then every locally range-valid candidate; successful and rejected conversions, invalid extreme entries, aliasing/clearing and empty success. |
| Domain and asymptotic edges | Embedded NUL/full bytes, high bits, signed 64-bit symbols, mixed-type non-narrowing, large unary and near-unary strings, million-symbol stress. Checked subprocess probes cover explicit preconditions. |

Quick/full/stress respectively cover binary strings through 5/8/9, all binary
pattern/text pairs through 3/5/6, 100/1200/8000 random pairs, every range-valid
array through 5/8/9, and 10000/200000/1000000-symbol adversaries. Full prefix
validation includes 46,234 locally bounded arrays over lengths 0..8; stress
adds the 362,880 length-9 candidates. The independent
partition oracle includes arbitrary-alphabet witnesses, not just binary ones.

## Commands and results

Recorded 2026-09-28 on Linux x86-64, Intel Core i9-11900H, GCC 16.2.1
(20260810), Python 3. The shared runner uses GNU++20 and optimized
`-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and sanitized
`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined
-fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie` builds,
with leak checking enabled.

| Command from repository root | Result |
|---|---|
| `python3 '96-Local Testing/07-Strings/01-prefixfunction_tester.py' --mode full --seed 20260928` | Optimized/checked: PASS, 528,876 checks each; initial 6 assertion probes passed. Sandbox LeakSanitizer failed on its ptrace restriction; this was an environment failure, not a reported algorithm error. |
| `python3 '96-Local Testing/07-Strings/01-prefixfunction_tester.py' --mode full --seed 20260928 --configuration ASan-UBSan` | Approved run outside the sandbox: PASS, 528,876 checks with leak checking. |
| `python3 '96-Local Testing/07-Strings/01-prefixfunction_tester.py' --mode stress --seed 20260928` | Final test source: PASS all three configurations, 3,315,194 checks each, all 7 assertion probes including the added length bound. |

Independent P016 review found and fixed mixed-alphabet narrowing in streaming KMP before completion.

P016 package verification (batches ST01 → ST02 → ST19, completed in that order on 2026-09-28; the C01/P002 prerequisite was already verified). Seed **20260928**, GCC **16.2.1 (20260810)**, GNU++20; Python test orchestration used **CPython 3.14.7**. Test oracles remain active under NDEBUG.

| Header | Executed feature mode | Checks per optimized / checked / ASan-UBSan configuration | Checked assertion probes |
|---|---|---|---|
| Prefix/KMP | Full, then final stress | 3,315,194 in stress | 7 |

All **43** assertion probes passed. Optimized builds use `-O2 -DNDEBUG`; checked builds use `_GLIBCXX_DEBUG`; sanitizer builds use ASan/UBSan with leak detection enabled. The sandbox's process tracer prevented LeakSanitizer from running, so the affected configurations were rerun with approved execution outside that tracer and passed. Per-header records distinguish those retries and final source coverage. Full was completed for all owned APIs; hash/trie/Manacher/run-length stress modes are available but were not executed. No other compiler/interpreter version is claimed as tested.

Final package integration passed:

- **99** current standalone/aggregate headers compiled.
- All aggregates linked and ran across multiple translation units in scalar and available AVX2 configurations.
- The existing standalone Workspace compiled in LOCAL and non-LOCAL configurations.
- The shared runner discovered and passed all **six** Strings suites in quick mode from `/tmp`, including their checked preconditions and hash Python oracle.
- Repository consistency passed with no errors, preserving the existing 428 targets, 335 batches and 226 packages.

Commands run from the repository root unless otherwise stated:

```bash
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

Shared discovery was run with working directory `/tmp`:

```bash
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 20260928 --no-integration
```

The `--no-integration` option avoids repeating the separately completed integration/consistency checks. Strings Basic/All aggregates and shared quick discovery now include the six implementations. The six inventory rows and the test coverage index link current evidence.

## Benchmarks

The root integration owner also checks standalone compilation, aggregates and
multiple translation units with the rest of P016; that evidence is recorded in
the P016 package record in [00-notes.md](00-notes.md) and under "Commands and results" above. The standard linear algorithms have no optional
arithmetic/ISA backend or measured dispatch threshold; large adversaries check
the claimed scalability, and no universal speed or benchmark superiority claim
is made. Other selected P016 algorithms use their justified direct linear/output-sensitive constructions without a competing specialization requiring a timing comparison.

## Sources

Inspected 2026-09-28; the code is independently implemented from the algorithmic
invariants, with no copied source text.

- [cp-algorithms, “Prefix function. Knuth–Morris–Pratt algorithm”](https://cp-algorithms.com/string/prefix-function.html), page update 2023-08-20: failure-chain proof, streaming storage, both prefix-count applications, periods and O(m*sigma) automaton DP. Our API uses a separate accepting row without appending a sentinel.
- Targeted prefix/KMP/Z searches in `OLD/algorithms.cpp`, `OLD/[1] algorithms.cpp`, and Team Notebook `algs.cpp`/`algsbetter.cpp` found no additional owned implementation; unrelated Knuth-DP/Berlekamp–Massey symbols were excluded.
- Exact conversion/validation follows the witness and interval arguments above, reviewed independently and checked against exhaustive alphabet-partition feasibility. No claim is made that the two tutorial sources describe these helpers.

## Limits and handoffs

Basic prefix/Z applications are complete here. Online append-only Z, richer
string reconstruction/minimum alphabet, substring period indexing and advanced
periodicity remain under their separately assigned ST24/ST25/ST11 headers.
Distinct-substring counting belongs to suffix indexes/ST04; the tutorials'
quadratic prefix/Z reduction is not added as a duplicate counting engine.

Future owners: ST03 can reuse `KmpMatcher` and `prefixAutomaton`. ST11 owns broader periodicity. ST25 owns constrained/minimum-alphabet reconstruction beyond ST01's exact feasibility and prefix/Z representation conversion. No ST01 feature or verification gap remains. Finite tests supplement the mathematical arguments; they do not prove collision-free hashing or allocate every maximum-size domain. No online submission or acceptance is claimed.
