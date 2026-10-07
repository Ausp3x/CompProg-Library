# Prefix functions and Z arrays — ST01

The two headers implement all ST01 inventory features. They use exact symbol
equality, accept empty sequences, and reserve no separator value. `string` and
`string_view` include embedded NUL and every byte; integer vectors may use the
full range of their element type. String positions count bytes, not Unicode
code points. Generic sequence equality must be a consistent equivalence
relation with constant-time indexing/comparison. Every sequence/array length
is strictly less than `INT_MAX`; returned indices and lengths are `int`.

## APIs and contracts

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
| `zFunction(s)` | `z[i]=LCP(s,s[i..n))`; **z[0]=n**. Empty returns `[]`. O(n) time and returned space. |
| `extendedZ(pattern,text)` | n LCP lengths between pattern and each text suffix; O(n+m) time/space. Empty pattern gives n zeros. It does not append a synthetic end position. |
| `zOccurrences(pattern,text)` | All overlapping starts, ascending; O(n+m) time and O(n+m+k) space. |
| `zBorders(z,include_full=false)`, `zPeriods(z)`, `zPeriod(z,whole=false)` | Same border/period conventions as prefix helpers, using an O(n) scan. Returned lists cost O(k) space; scalar period uses O(1). |
| `validPrefixFunction(pi)`, `validZFunction(z)` | Exact feasibility over an unrestricted integer alphabet; O(n) time/space. Empty arrays are valid. These impose no fixed-alphabet or minimum-alphabet guarantee. |
| `prefixToZ(pi,z)`, `zToPrefix(z,pi)` | Exact checked conversions, O(n) time/space; bool success distinguishes valid empty input from failure. Output may alias input and is cleared on failure. `z[0]=n` is strictly enforced. |

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

The Z algorithm maintains a rightmost prefix-matching interval `[l,r)`.
An interior LCP is known up to `min(z[i-l],r-i)`; further comparisons extend the
right boundary, which advances at most n times. Extended KMP uses pattern Z
values inside a text match interval and the same boundary argument. Keeping
the two sequences separate avoids sentinel/alphabet restrictions. A positive
period p is equivalent to a border of length n-p, or `z[p]=n-p`.

For prefix validation, a positive entry forces the endpoint equality
`s[i]=s[pi[i]-1]`; a zero entry receives a fresh integer symbol. Every equality
required by a feasible prefix array is generated by these endpoint equalities
(induct along the border's positions). Splitting otherwise unconstrained
classes cannot destroy a required border. Recomputing the prefix function
therefore accepts exactly feasible arrays and detects contradictory extra
or missing borders. This internal witness is not a fixed/minimum-alphabet or
lexicographically minimum reconstruction interface; ST25 owns those contracts.

For Z-to-prefix conversion, a match interval starting at i implies prefix
lengths 1..z[i] at its successive endpoints. Earlier starts provide longer
prefixes. Assign backwards only while an endpoint is unset; after a filled
endpoint, all earlier endpoints in that interval were already covered by an
earlier start. Each assignment fills a fresh entry, plus at most one failed
check per start, so the work is linear. Build the canonical prefix witness and
recompute its Z array to reject locally bounded but globally inconsistent
inputs. Prefix-to-Z uses the same exact witness check. Temporaries prevent
input/output aliasing from invalidating reads.

## Sources and legacy accounting

Inspected 2026-09-28; the code is independently implemented from the algorithmic
invariants, with no copied source text.

- [cp-algorithms, “Prefix function. Knuth–Morris–Pratt algorithm”](https://cp-algorithms.com/string/prefix-function.html), page update 2023-08-20: failure-chain proof, streaming storage, both prefix-count applications, periods and O(m*sigma) automaton DP. Our API uses a separate accepting row without appending a sentinel.
- [cp-algorithms, “Z-function and its calculation”](https://cp-algorithms.com/string/z-function.html): rightmost match interval, linear proof and matching/period applications. Its conventional z[0]=0 is deliberately changed to the explicit full-LCP convention here.
- [OI Wiki, “Z 函数（扩展 KMP）”](https://oi-wiki.org/string/z-func/), page update 2026-01-07: independent presentation of box reuse and period/matching applications. The two-sequence extended-LCP routine applies the same invariant without concatenation. This reference also uses z[0]=0.
- [Nyaan, `string/z-algorithm.hpp`](https://github.com/NyaanNyaan/library/blob/master/string/z-algorithm.hpp): inspected the complete generic-container implementation, its empty return and z[0]=n, and its alternative interior-box copying loop. This provides an independently maintained implementation comparison; OI Wiki explicitly derives its exposition from cp-algorithms/e-maxx.
- [AtCoder Library, `atcoder/string.hpp`, `z_algorithm`](https://github.com/atcoder/ac-library/blob/master/atcoder/string.hpp): inspected only the Z overloads, confirming the generic equality algorithm, empty return and full-LCP convention. Its source is an independent behavior comparison, not a dependency or copied implementation.
- Preserved `OLD/Team Notebook/src/misc/old_zalgo.cpp`: static Z with final z[0]=n, fixed global storage and reliance on initially zeroed array entries. The maintained header accounts for the static-Z feature and its full-LCP convention, returns an empty owning array for empty input, supports repeated independent calls, and leaves the original bytes unchanged.
- Targeted prefix/KMP/Z searches in `OLD/algorithms.cpp`, `OLD/[1] algorithms.cpp`, and Team Notebook `algs.cpp`/`algsbetter.cpp` found no additional owned implementation; unrelated Knuth-DP/Berlekamp–Massey symbols were excluded.
- Exact conversion/validation follows the witness and interval arguments above, reviewed independently and checked against exhaustive alphabet-partition feasibility. No claim is made that the two tutorial sources describe these helpers.

Basic prefix/Z applications are complete here. Online append-only Z, richer
string reconstruction/minimum alphabet, substring period indexing and advanced
periodicity remain under their separately assigned ST24/ST25/ST11 headers.
Distinct-substring counting belongs to suffix indexes/ST04; the tutorials'
quadratic prefix/Z reduction is not added as a duplicate counting engine.

## Verification

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
| Extended LCP and Z matching | All bounded pairs and random alphabets versus direct LCP and substring equality. |
| Validation/conversion | Enumerate every alphabet equality partition to derive the full feasible array set, then every locally range-valid candidate; successful and rejected conversions, invalid extreme entries, aliasing/clearing and empty success. |
| Domain and asymptotic edges | Embedded NUL/full bytes, high bits, signed 64-bit symbols, mixed-type non-narrowing, large unary and near-unary strings, million-symbol stress. Checked subprocess probes cover explicit preconditions. |

Quick/full/stress respectively cover binary strings through 5/8/9, all binary
pattern/text pairs through 3/5/6, 100/1200/8000 random pairs, every range-valid
array through 5/8/9, and 10000/200000/1000000-symbol adversaries. Full prefix
validation includes 46,234 locally bounded arrays over lengths 0..8; stress
adds the 362,880 length-9 candidates. Z uses the same count because its first
entry is fixed and the remaining ranges have factorial product. The independent
partition oracle includes arbitrary-alphabet witnesses, not just binary ones.

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
| `python3 '96-Local Testing/07-Strings/02-z_tester.py' --mode stress --seed 20260928` | Optimized/checked: PASS, 1,410,995 checks each; sandbox LeakSanitizer hit the same ptrace restriction. |
| `python3 '96-Local Testing/07-Strings/02-z_tester.py' --mode full --seed 20260928 --configuration checked` | Final test source: PASS, 174,755 checks and all 3 added length-bound assertion probes. |
| `python3 '96-Local Testing/07-Strings/02-z_tester.py' --mode stress --seed 20260928 --configuration ASan-UBSan` | Approved run outside the sandbox: PASS, 1,410,995 checks with leak checking. |

The root integration owner also checks standalone compilation, aggregates and
multiple translation units with the rest of P016; that evidence is recorded in
the package integration note. The standard linear algorithms have no optional
arithmetic/ISA backend or measured dispatch threshold; large adversaries check
the claimed scalability, and no universal speed or benchmark superiority claim
is made. No ST01 feature or verification gap remains.
