# 04-trie.hpp — evidence

`04-trie.hpp` (batch ST02, package P016) provides a full-byte multiset trie with prefix/lexicographic traversal and reclaimed nodes. It belongs to the P016 ST02 ownership slice (`04-trie.hpp`, `05-manacher.hpp` and their mirrored tests). String hashing has separate evidence in [03-stringhash.md](03-stringhash.md). Implementation began after ST01 feature verification; no ST03 palindrome-query implementation is included here.

## Contracts

### Trie

`Trie` stores a multiset of byte strings, including the empty key, embedded NUL and all 256 byte values. Its ordered sparse edges compare bytes as unsigned values; lexicographic enumeration places a key before its extensions. `insert(s,k=1)` inserts a positive signed 64-bit multiplicity, `count(s)` returns terminal multiplicity, `search(s)` tests membership, and `countPrefix(s)` sums multiplicities of keys beginning with `s`. The empty prefix counts the complete dictionary. `erase(s,k=1)` removes exactly `k` copies or returns false without mutation when the key has too few copies. `size()` includes multiplicities; `empty()` and `clear()` have their ordinary container meanings. The supported total is at most `INT64_MAX`, every supplied key/prefix length is below `INT_MAX`, and peak live prefix nodes including the root must fit `INT_MAX`.

`forEach(prefix, callback)` invokes `callback(string_view word, lng multiplicity)` once per distinct matching key, in unsigned-byte lexicographic order. The overload without a prefix visits the whole dictionary. Return true from a callback to continue, false to stop; the outer result is false on cancellation even at the final key. A missing prefix completes true without visits. A word view is borrowed only during its callback. Callbacks may perform nested reads/enumerations and propagate exceptions; they must not mutate the trie. The implementation uses an explicit stack, so long keys do not consume recursive call-stack space.

With maximum fanout `sigma <= 256`, a key of length `m` takes `O(m * log(sigma + 1))` lookup/update work; insertion and vector-stack/free-list growth use amortized allocation bounds. Enumeration takes `O(m * log(sigma + 1) + V)` excluding callback work, for `V` visited prefix-subtree nodes, and `O(m + h)` temporary space for maximum suffix depth `h`. Erase uses `O(m)` temporary space. Sparse edges and recycled slots take `O(P)` stored space where `P` is peak simultaneously live prefix nodes. Erase prunes dead paths and reuses their slots; `clear()` releases all edge allocations but retains vector capacities. Copying costs `O(P)` and creates independent state. Moves transfer state; clear or assign a moved-from trie before using its other operations. Public fields and allocation/traversal helpers expose contest implementation state, not additional independently mutable contracts.

The algorithms use the contest profile, with no special ISA or modular-reduction dependency. Sparse trie transitions preserve deterministic bounds without a 256-integer row at every sparse node; Manacher avoids a transformed string and sentinels. No competing implementation or timing threshold is selected, so comparative benchmarking is not required. Large structural cases check practical allocation and linear traversal; they are correctness evidence, not a performance claim.

## Correctness arguments

Every live trie edge appends exactly its byte to the root-to-node key. A node's `terminal` is that key's multiplicity; its `pass` equals `terminal` plus child `pass` values. Insertion changes precisely one root-to-terminal path. Checking the root total before incrementing prevents overflow of every subordinate count. Erase first checks the entire terminal multiplicity, then subtracts along exactly the same path. A zero-count suffix of that path has no live branch, so removing its edges and recycling its already emptied nodes preserves every other key. Iterative depth-first traversal visits terminal keys before increasing outgoing byte labels, which is lexicographic order. Reused slots are unreachable and have zero counters and no remaining children.

## Feature-to-test map

| Header / feature | Independent coverage |
|---|---|
| Trie insert, terminal/prefix counts, search, size, empty, all-or-nothing erase | Every multiset over all binary keys of length at most 2 with multiplicities 0..2 (2187 states); independent ordered-map oracle, absent/insufficient erase and partial/final removals; seeded operation streams. |
| Trie unsigned alphabet and enumeration | Empty key, all 256 one-byte keys and complementary two-byte keys, NUL, prefix filtering, independently sorted expected records, one callback per terminal multiplicity. |
| Trie callback lifetime/state and cancellation | Stops at first/second/middle/last record, nested enumeration/lookup, propagated callback exception followed by rechecking contents. |
| Trie mutation lifecycle and storage | Copy independence, move destination, reset moved-from state, clear, root/child/subtree/free-slot invariants, 100000-byte keys, repeated erase/reinsert without slot growth. |
| Trie count domain | Maximum `INT64_MAX` total/terminal counts and whole-count removal; five checked assertion probes cover nonpositive insert/erase and total overflow. |

Each mirrored Python entry runs from any working directory via the shared string runner. Quick/full/stress scope is recorded in each entry; optimized `-DNDEBUG`, checked `_GLIBCXX_DEBUG` and ASan/UBSan configurations retain independent non-assertion test oracles. Invalid preconditions remain caller requirements in release builds. Tests do not allocate impractical `INT_MAX`-length strings or node pools; overflow safety at those indexing limits is established by the arithmetic argument above rather than an allocation claim.

## Commands and results

Full test command (seed `20260928`):

```text
python3 '96-Local Testing/07-Strings/04-trie_tester.py' --mode full --seed 20260928
```

On 2026-09-28, the full suite passed with GCC 16.2.1 (20260810), GNU++20:

| Header | Per-configuration result | Checked precondition probes |
|---|---|---|
| Trie | 2,077,118 checks, 2187 exhaustive dictionaries, 7000 seeded operations, 100000-byte deep key | 5 passed |

The configurations were optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie`. Leak detection remained enabled. The initial sandbox sanitizer processes encountered LeakSanitizer's fatal ptrace incompatibility at shutdown; the sanitizer configurations were rerun outside the sandbox, passed, and reported the same check counts:

```text
python3 '96-Local Testing/07-Strings/04-trie_tester.py' --mode full --seed 20260928 --configuration ASan-UBSan
```

Header-alone, aggregate/multiple-translation-unit and repository consistency verification are recorded by the P016 integration owner. No owned feature or algorithm-verification gap remains in these two headers; finite testing is supported by the correctness arguments above, not a claim of proof by enumeration.

P016 package verification (batches ST01 → ST02 → ST19, completed in that order on 2026-09-28; the C01/P002 prerequisite was already verified). Seed **20260928**, GCC **16.2.1 (20260810)**, GNU++20; Python test orchestration used **CPython 3.14.7**. Test oracles remain active under NDEBUG.

| Header | Executed feature mode | Checks per optimized / checked / ASan-UBSan configuration | Checked assertion probes |
|---|---|---|---|
| Trie | Full | 2,077,118 | 5 |

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

- [OI Wiki, 字典树 (Trie)](https://oi-wiki.org/string/trie/): definition, byte/character transition model, basic insertion/search implementation and retrieval applications. Binary integer/XOR and persistent-trie sections are separate ownership; they were not adopted into this dictionary.
- [cp-algorithms, Aho-Corasick — Construction of the trie](https://cp-algorithms.com/string/aho_corasick.html#construction-of-the-trie): root/prefix/terminal invariant and dense-array versus map-transition time/space analysis. Only trie construction is used; no Aho automaton claim is made.
- Preserved `OLD/Team Notebook/src/algs.cpp` lines 2374–2397 and `algsbetter.cpp` lines 3332–3402: existing lowercase-26 trie insertion, search, terminal/pass counters and erase-one-if-present behavior. Those features are covered here with a full byte alphabet, explicit bulk multiplicities/erase status, enumeration and storage reuse. No original bytes were changed. The two root monoliths contain no active trie implementation (`OLD/[1] algorithms.cpp` has a TODO).

## Limits and handoffs

Compressed radix/persistent string dictionaries remain ST22 `25-radixtrie.hpp`; binary integer/XOR tries remain Data Structures. These are separate scheduled features, not missing functionality of this header. Finite tests supplement the mathematical arguments; they do not allocate every maximum-size domain. No online submission or acceptance is claimed.
