# 05-manacher.hpp — evidence

`05-manacher.hpp` (batch ST02, package P016) provides generic Manacher radii and maximal/nested interval reconstruction. It belongs to the P016 ST02 ownership slice (`04-trie.hpp`, `05-manacher.hpp` and their mirrored tests). String hashing has separate evidence in [03-stringhash.md](03-stringhash.md). Implementation began after ST01 feature verification; no ST03 palindrome-query implementation is included here.

## Contracts

### Manacher

`Manacher(s)` takes any sequence with random access, `.size()` and equality comparison, of length at most `INT_MAX`. `string`, `string_view` and integer vectors are supported; pass literals as `string_view`. The default constructor represents the empty sequence. It stores no reference to the input. `odd[i]` includes its center and denotes `[i-odd[i]+1, i+odd[i])`. `even[i]` denotes `[i-even[i], i+even[i])` around gap `i`; there are `n+1` gap entries, including zero radii at both endpoint gaps. Thus a singleton has `odd={1}, even={0,0}`, and empty input has `odd={}, even={0}`.

`oddInterval(i,k=-1)` and `evenInterval(i,k=-1)` return half-open ranges. `oddInclusive` and `evenInclusive` return inclusive endpoints. Radius `-1` selects the maximum; an explicit radius must be between 1 and `odd[i]`, or between 0 and `even[i]`, respectively. Empty even intervals return `[i,i)` or inclusive `[i,i-1]`, including `[0,-1]` for the empty input. These endpoints are interval conventions, not absence sentinels. Construction takes `O(n)` time and stored space, with `O(1)` auxiliary state beyond the returned arrays; each reconstruction takes `O(1)`. Radius arrays are read-only after construction. Copy/move transfer array values; assign a moved-from object before invoking radius queries.

The algorithms use the contest profile, with no special ISA or modular-reduction dependency. Sparse trie transitions preserve deterministic bounds without a 256-integer row at every sparse node; Manacher avoids a transformed string and sentinels. No competing implementation or timing threshold is selected, so comparative benchmarking is not required. Large structural cases check practical allocation and linear traversal; they are correctness evidence, not a performance claim.

## Correctness arguments

For either parity, Manacher maintains the rightmost maximal palindrome already found. If the next center lies inside it, reflection provides an already known radius, clipped at the right boundary. A smaller reflected palindrome stops at a reflected mismatch; one reaching the boundary can be extended only by comparing new symmetric symbols. Every successful comparison beyond the current boundary advances that boundary, giving `O(n)` total extension work. Odd and even loops apply this invariant to character centers and gap centers separately. Boundary differences and `l + (r-i)` mirror indices avoid overflowing sums of two absolute indices, including the documented `INT_MAX` size domain. Reconstruction is exactly the radius definition, and smaller radii remain palindromes by symmetric deletion.

## Feature-to-test map

| Header / feature | Independent coverage |
|---|---|
| Manacher radii and reconstruction | All ternary strings through length 8; independent enumeration of every half-open substring with direct palindrome checks, all four maximum/nested reconstruction methods, endpoint gaps and zero radii. |
| Manacher alphabets and lifecycle | 2000 seeded byte cases and corresponding signed 64-bit extreme-value alphabets, all 256 bytes, palindromic/adversarial strings, default empty, copy/move/assignment. |
| Manacher linear-size shapes and contracts | Unary/alternating length 300000 with independently known radii; ten checked center/radius assertion probes. |

Each mirrored Python entry runs from any working directory via the shared string runner. Quick/full/stress scope is recorded in each entry; optimized `-DNDEBUG`, checked `_GLIBCXX_DEBUG` and ASan/UBSan configurations retain independent non-assertion test oracles. Invalid preconditions remain caller requirements in release builds. Tests do not allocate impractical `INT_MAX`-length strings or node pools; overflow safety at those indexing limits is established by the arithmetic argument above rather than an allocation claim.

## Commands and results

Full test command (seed `20260928`):

```text
python3 '96-Local Testing/07-Strings/05-manacher_tester.py' --mode full --seed 20260928
```

On 2026-09-28, the full suite passed with GCC 16.2.1 (20260810), GNU++20:

| Header | Per-configuration result | Checked precondition probes |
|---|---|---|
| Manacher | 3,200,052 checks, 13,842 independently verified small cases, 300000-symbol unary/alternating cases | 10 passed |

The configurations were optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie`. Leak detection remained enabled. The initial sandbox sanitizer processes encountered LeakSanitizer's fatal ptrace incompatibility at shutdown; the sanitizer configurations were rerun outside the sandbox, passed, and reported the same check counts:

```text
python3 '96-Local Testing/07-Strings/05-manacher_tester.py' --mode full --seed 20260928 --configuration ASan-UBSan
```

Header-alone, aggregate/multiple-translation-unit and repository consistency verification are recorded by the P016 integration owner. No owned feature or algorithm-verification gap remains in these two headers; finite testing is supported by the correctness arguments above, not a claim of proof by enumeration.

P016 package verification (batches ST01 → ST02 → ST19, completed in that order on 2026-09-28; the C01/P002 prerequisite was already verified). Seed **20260928**, GCC **16.2.1 (20260810)**, GNU++20; Python test orchestration used **CPython 3.14.7**. Test oracles remain active under NDEBUG.

| Header | Executed feature mode | Checks per optimized / checked / ASan-UBSan configuration | Checked assertion probes |
|---|---|---|---|
| Manacher | Full | 3,200,052 | 10 |

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

No benchmark: no competing implementation or timing threshold is selected (see the contracts). Other selected P016 algorithms use their justified direct linear/output-sensitive constructions without a competing specialization requiring a timing comparison.

## Sources

Inspected 2026-09-28; implementation is independent rather than a source-code port.

- [cp-algorithms, Manacher's Algorithm — Finding all sub-palindromes](https://cp-algorithms.com/string/manacher.html): odd/even definitions, reflected-radius clipping, boundary extension and linear-time proof. This implementation uses two direct parity loops without sentinel symbols.
- [KACTL, Manacher.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/Manacher.h), credited there to Codeforces user adamant: inspected the compact two-parity implementation and its radius convention. KACTL's odd half-length excludes the center; the API here deliberately includes it and documents that difference.
- No legacy Manacher implementation was found in the inspected code files (`OLD/Team Notebook/src/algs.cpp`, `algsbetter.cpp` and the two root monoliths).

## Limits and handoffs

ST02 does not own exact static substring-palindrome checks or longest-palindrome extraction: ST03 `08-palindrome_queries.hpp` can use these radii and conversions. Hash-based palindrome checks must remain explicitly probabilistic there. Finite tests supplement the mathematical arguments; they do not allocate every maximum-size domain. No online submission or acceptance is claimed.
