# 05-manacher.hpp — evidence

`05-manacher.hpp` (batch ST02, package P016) provides generic Manacher radii and maximal/nested interval reconstruction. It belongs to the P016 ST02 ownership slice (`04-trie.hpp`, `05-manacher.hpp` and their mirrored tests).

## Contracts

### Manacher

`Manacher(s)` takes any sequence with random access, `.size()` and equality comparison, of length at most `INT_MAX`. `string`, `string_view` and integer vectors are supported; pass literals as `string_view`. The default constructor represents the empty sequence. It stores no reference to the input. `odd[i]` includes its center and denotes `[i-odd[i]+1, i+odd[i])`. `even[i]` denotes `[i-even[i], i+even[i])` around gap `i`; there are `n+1` gap entries, including zero radii at both endpoint gaps. Thus a singleton has `odd={1}, even={0,0}`, and empty input has `odd={}, even={0}`.

`oddInterval(i,k=-1)` and `evenInterval(i,k=-1)` return half-open ranges. `oddInclusive` and `evenInclusive` return inclusive endpoints. Radius `-1` selects the maximum; an explicit radius must be between 1 and `odd[i]`, or between 0 and `even[i]`, respectively. Empty even intervals return `[i,i)` or inclusive `[i,i-1]`, including `[0,-1]` for the empty input. These endpoints are interval conventions, not absence sentinels. Construction takes `O(n)` time and stored space, with `O(1)` auxiliary state beyond the returned arrays; each reconstruction takes `O(1)`. Radius arrays are read-only after construction. Copy/move transfer array values; assign a moved-from object before invoking radius queries.

The algorithms use the contest profile, with no special ISA or modular-reduction dependency. Manacher avoids a transformed string and sentinels.

Correctness: for either parity, Manacher maintains the rightmost maximal palindrome already found. If the next center lies inside it, reflection provides an already known radius, clipped at the right boundary. A smaller reflected palindrome stops at a reflected mismatch; one reaching the boundary can be extended only by comparing new symmetric symbols. Every successful comparison beyond the current boundary advances that boundary, giving `O(n)` total extension work. Odd and even loops apply this invariant to character centers and gap centers separately. Boundary differences and `l + (r-i)` mirror indices avoid overflowing sums of two absolute indices, including the documented `INT_MAX` size domain. Reconstruction is exactly the radius definition, and smaller radii remain palindromes by symmetric deletion.

## Feature-to-test map

| Header / feature | Independent coverage |
|---|---|
| Manacher radii and reconstruction | All ternary strings through length 8; independent enumeration of every half-open substring with direct palindrome checks, all four maximum/nested reconstruction methods, endpoint gaps and zero radii. |
| Manacher alphabets and lifecycle | 2000 seeded byte cases and corresponding signed 64-bit extreme-value alphabets, all 256 bytes, palindromic/adversarial strings, default empty, copy/move/assignment. |
| Manacher linear-size shapes and contracts | Unary/alternating length 300000 with independently known radii; ten checked center/radius assertion probes. |

Each mirrored Python entry runs from any working directory via the shared string runner. Quick/full/stress scope is recorded in each entry; optimized `-DNDEBUG`, checked `_GLIBCXX_DEBUG` and ASan/UBSan configurations retain independent non-assertion test oracles. Invalid preconditions remain caller requirements in release builds. Tests do not allocate impractical `INT_MAX`-length strings or node pools; overflow safety at those indexing limits is established by the arithmetic argument above rather than an allocation claim.

## Commands and results

P016 verification run, 2026-09-28: Linux x86-64 (i9-11900H), GCC 16.2.1 (20260810),
GNU++20, CPython 3.14.7, seed 20260928. The shared runner builds optimized
`-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan (leak checking on)
configurations; test oracles stay active under NDEBUG. LeakSanitizer cannot run
under the sandbox process tracer, so sanitizer configurations ran outside it.

| Command from repository root | Result |
|---|---|
| `python3 '96-Local Testing/07-Strings/05-manacher_tester.py' --mode full --seed 20260928` | PASS, all three configurations, 3,200,052 checks each (13,842 small cases, 300000-symbol unary/alternating cases); 10 assertion probes |
| `python3 '96-Local Testing/02-integration.py'` | PASS: 99 standalone/aggregate headers, multi-TU scalar/AVX2 aggregates, Workspace LOCAL and non-LOCAL |
| `python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 20260928 --no-integration` (from `/tmp`) | PASS, all six P016 Strings suites |
| `python3 '96-Local Testing/03-consistency.py'` | no errors |

No other compiler, including the `g++-14` floor, is recorded as tested.

Stress mode exists but was not run.

## Benchmarks

No competing implementation or timing threshold is selected, so no benchmark is
recorded. Large structural cases are correctness evidence, not a performance claim.

## Sources

Inspected 2026-09-28; implementation is independent rather than a source-code port.

- [cp-algorithms, Manacher's Algorithm — Finding all sub-palindromes](https://cp-algorithms.com/string/manacher.html): odd/even definitions, reflected-radius clipping, boundary extension and linear-time proof. This implementation uses two direct parity loops without sentinel symbols.
- [KACTL, Manacher.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/Manacher.h), credited there to Codeforces user adamant: inspected the compact two-parity implementation and its radius convention. KACTL's odd half-length excludes the center; the API here deliberately includes it and documents that difference.
- No legacy Manacher implementation was found in the inspected code files (`OLD/Team Notebook/src/algs.cpp`, `algsbetter.cpp` and the two root monoliths).

## Limits and handoffs

- Open `/reaudit-review` findings for P016 ([p016.md](<../../00-Guidelines/23-Reaudit Findings/p016.md>)), not yet resolved:
  - 13: struct complexity comment omits the U component.
  - 14: constructor and all four interval methods break the closing-brace rule.

ST02 does not own exact static substring-palindrome checks or longest-palindrome extraction: ST03 `08-palindrome_queries.hpp` can use these radii and conversions. Hash-based palindrome checks must remain explicitly probabilistic there. Finite tests supplement the correctness arguments; no maximum-size domain is allocated.
