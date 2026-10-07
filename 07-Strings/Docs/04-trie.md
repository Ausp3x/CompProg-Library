# 04-trie.hpp — evidence

`04-trie.hpp` (batch ST02, package P016) provides a full-byte multiset trie with prefix/lexicographic traversal and reclaimed nodes. It belongs to the P016 ST02 ownership slice (`04-trie.hpp`, `05-manacher.hpp` and their mirrored tests).

## Contracts

### Trie

`Trie` stores a multiset of byte strings, including the empty key, embedded NUL and all 256 byte values. Its ordered sparse edges compare bytes as unsigned values; lexicographic enumeration places a key before its extensions. `insert(s,k=1)` inserts a positive signed 64-bit multiplicity, `count(s)` returns terminal multiplicity, `search(s)` tests membership, and `countPrefix(s)` sums multiplicities of keys beginning with `s`. The empty prefix counts the complete dictionary. `erase(s,k=1)` removes exactly `k` copies or returns false without mutation when the key has too few copies. `size()` includes multiplicities; `empty()` and `clear()` have their ordinary container meanings. The supported total is at most `INT64_MAX`, every supplied key/prefix length is below `INT_MAX`, and peak live prefix nodes including the root must fit `INT_MAX`.

`forEach(prefix, callback)` invokes `callback(string_view word, lng multiplicity)` once per distinct matching key, in unsigned-byte lexicographic order. The overload without a prefix visits the whole dictionary. Return true from a callback to continue, false to stop; the outer result is false on cancellation even at the final key. A missing prefix completes true without visits. A word view is borrowed only during its callback. Callbacks may perform nested reads/enumerations and propagate exceptions; they must not mutate the trie. The implementation uses an explicit stack, so long keys do not consume recursive call-stack space.

With maximum fanout `sigma <= 256`, a key of length `m` takes `O(m * log(sigma + 1))` lookup/update work; insertion and vector-stack/free-list growth use amortized allocation bounds. Enumeration takes `O(m * log(sigma + 1) + V)` excluding callback work, for `V` visited prefix-subtree nodes, and `O(m + h)` temporary space for maximum suffix depth `h`. Erase uses `O(m)` temporary space. Sparse edges and recycled slots take `O(P)` stored space where `P` is peak simultaneously live prefix nodes. Erase prunes dead paths and reuses their slots; `clear()` releases all edge allocations but retains vector capacities. Copying costs `O(P)` and creates independent state. Moves transfer state; clear or assign a moved-from trie before using its other operations. Public fields and allocation/traversal helpers expose contest implementation state, not additional independently mutable contracts.

The algorithms use the contest profile, with no special ISA or modular-reduction dependency. Sparse trie transitions preserve deterministic bounds without a 256-integer row at every sparse node.

Correctness: every live trie edge appends exactly its byte to the root-to-node key. A node's `terminal` is that key's multiplicity; its `pass` equals `terminal` plus child `pass` values. Insertion changes precisely one root-to-terminal path. Checking the root total before incrementing prevents overflow of every subordinate count. Erase first checks the entire terminal multiplicity, then subtracts along exactly the same path. A zero-count suffix of that path has no live branch, so removing its edges and recycling its already emptied nodes preserves every other key. Iterative depth-first traversal visits terminal keys before increasing outgoing byte labels, which is lexicographic order. Reused slots are unreachable and have zero counters and no remaining children.

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

P016 verification run, 2026-09-28: Linux x86-64 (i9-11900H), GCC 16.2.1 (20260810),
GNU++20, CPython 3.14.7, seed 20260928. The shared runner builds optimized
`-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan (leak checking on)
configurations; test oracles stay active under NDEBUG. LeakSanitizer cannot run
under the sandbox process tracer, so sanitizer configurations ran outside it.

| Command from repository root | Result |
|---|---|
| `python3 '96-Local Testing/07-Strings/04-trie_tester.py' --mode full --seed 20260928` | PASS, all three configurations, 2,077,118 checks each (2187 exhaustive dictionaries, 7000 seeded operations, 100000-byte key); 5 assertion probes |
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

- [OI Wiki, 字典树 (Trie)](https://oi-wiki.org/string/trie/): definition, byte/character transition model, basic insertion/search implementation and retrieval applications. Binary integer/XOR and persistent-trie sections are separate ownership; they were not adopted into this dictionary.
- [cp-algorithms, Aho-Corasick — Construction of the trie](https://cp-algorithms.com/string/aho_corasick.html#construction-of-the-trie): root/prefix/terminal invariant and dense-array versus map-transition time/space analysis. Only trie construction is used; no Aho automaton claim is made.
- Preserved `OLD/Team Notebook/src/algs.cpp` lines 2374–2397 and `algsbetter.cpp` lines 3332–3402: existing lowercase-26 trie insertion, search, terminal/pass counters and erase-one-if-present behavior. Those features are covered here with a full byte alphabet, explicit bulk multiplicities/erase status, enumeration and storage reuse. No original bytes were changed. The two root monoliths contain no active trie implementation (`OLD/[1] algorithms.cpp` has a TODO).

## Limits and handoffs

- Open `/reaudit-review` findings for P016 ([p016.md](<../../00-Guidelines/23-Reaudit Findings/p016.md>)), not yet resolved:
  - 5: `Node`, `newNode` and `findNode` are inventory operations without feature-map rows or direct tests, while the header calls them internal.
  - 11: complexity comment uses `m` and `sigma` instead of the standard `L` and `S`.
  - 12: function bodies close on their own line (closing-brace rule).

Compressed radix/persistent string dictionaries remain ST22 `25-radixtrie.hpp`; binary integer/XOR tries remain Data Structures. These are separate scheduled features, not missing functionality of this header. Finite tests supplement the correctness arguments; no maximum-size domain is allocated.
