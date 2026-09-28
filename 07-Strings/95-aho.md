# Aho–Corasick — ST03 / P017

`06-aho.hpp` implements the entire selected static multi-pattern inventory. The
dictionary supports all bytes, including NUL and bytes 128–255, duplicate pattern
IDs, and empty patterns. Dynamic dictionary changes after construction belong to
the separately planned `34-dynamicaho.hpp`.

## Contract and use

`AhoCorasick ac(patterns, dense)` constructs a ready dictionary; `dense` defaults
to false. Alternatively call `add(pattern)` on a fresh object, retaining its
insertion-order ID, then `build(dense)`. Adding after building violates a
precondition. Repeating `build` changes/rebuilds the representation while keeping
IDs and states; switching to sparse releases the dense table. `clear` restarts
both. Default copy/move owns all storage. A
moved-from object must be cleared or assigned before further queries. Pattern and
text views need remain valid only during their call. Allocation failure follows
the standard containers' exceptions; no transactional recovery is promised.

Every pattern/text length, number of IDs and number of trie states must be less
than `INT_MAX`; practical allocation limits also apply. Counts are exact, with
`int` per-boundary counts and `lng` accumulated/per-pattern counts. The product
of the maximum number of boundaries and pattern IDs fits signed 64 bits under
these limits. Root is state 0 and recognizes the empty prefix. An empty pattern
matches at every boundary `0..n`, including before the first byte. Duplicates get
separate results and each contributes to aggregate counts.

| API | Meaning |
|---|---|
| `step(state, byte)` | Longest dictionary-prefix suffix after appending the byte; unknown alphabet bytes return root. Keep the state across text chunks for streaming. |
| `nodes[u].link`, `nodes[u].out`, `terminal[id]` | Longest proper prefix-state suffix, nearest proper terminal suffix (`-1` if absent), and terminal state for each ID. Root links to itself and its output link is `-1`. Treat these exposed fields as read-only. |
| `matchCount(state)` | Number of IDs ending at this state, including every terminal suffix and empty ID. |
| `forEachOutput(state, callback)` | IDs longest-pattern first, duplicate IDs in insertion order. |
| `forEachMatch(text, callback)` | Calls `(id,l,r)` with half-open `[l,r)`, increasing end boundary, then the output order above. |
| `countPatterns(text)` | One occurrence count per insertion ID. |
| `countPositions(text)` | Exactly `n+1` counts, indexed by exclusive end boundary. |
| `countMatches(text)` | Total number of matches, without output materialization. |
| `stateCounts(text)` | Occurrence count of every trie-prefix string, root count `n+1`. |
| `suffixAggregate(values, combine)` | Fold each state's original value with all proper suffix states, nearest first and root once. The associative combine may be noncommutative. Caller supplies safe arithmetic and value lifetimes. |
| `forbidden(state)`, `safeStep(state, byte)`, `avoids(text)` | Reject states with any output. `safeStep` returns absorbing sink `-1` for already-bad or newly-bad states. Empty patterns forbid the root and all texts. |

Callbacks return bool; false cancels immediately and returns false, including if
it occurs at the last match. Callbacks may nest const queries but must not mutate
the dictionary. When streaming, report the root's empty outputs once before the
first chunk, then report outputs after each consumed byte; do not count a chunk
boundary twice.

For forbidden-language counting, initialize `dp[0] = !ac.forbidden(0)`. For each
requested output position, state and allowed byte, add `dp[u]` to the next layer
at `ac.safeStep(u,c)` when that state is nonnegative. Choose exact or modular
count arithmetic explicitly. This yields the automaton needed for counting,
digit DP, or witness reconstruction; any externally selected subset of bytes is
allowed, including bytes absent from the pattern alphabet. A dense build gives
constant-time arbitrary DP transitions. Enumeration tests independently check
this application for every tested small binary dictionary and lengths 0–7.

## Complexity and correctness

Let `L` be total inserted pattern length, `k` number of IDs, `V` number of states,
`sigma <= 256` distinct pattern bytes, `n` text length and `z` reported matches.
Sparse construction takes `O(k + L * log(sigma+2))` time and `O(V+k)` stored
memory. Inserting uses ordered sparse maps; building follows failure links for
each trie edge. It is **not** claimed linear in `V`: on each root-to-leaf path,
failure-depth decreases telescope against increases of at most one per edge.
Summing the resulting bound over trie leaves costs at most the sum of inserted
pattern lengths. The build is iterative and does not use recursion proportional
to pattern length.

Dense construction takes `O(k + L * log(sigma+2) + V * sigma)` time and
`O(V * (sigma+1) + k)` memory. It retains the sparse trie and adds exactly
`4 * V * sigma` bytes of completed-transition entries on the supported platform;
the byte-to-column table compresses only actually used symbols. An empty
alphabet allocates no transition entries. Sparse queries use failure walks:
a whole scan starting at root takes `O(n * log(sigma+2))`, while a single query
from an arbitrary state can cost `O((h+1) * log(sigma+2))`, `h` the longest pattern
length. Dense scans take `O(n)` and individual transitions take `O(1)`.

The state invariant is the longest prefix of any pattern that is a suffix of
processed text. Failure links drop to the longest proper such suffix; breadth
first construction knows every required shorter state's transitions first.
Following terminal-only output links therefore enumerates every matching ID
once, including the empty terminal at root. The output count recurrence is its
own terminal multiplicity plus the failure state's count. Counts need no output
list and retain scan complexity even when `z` is quadratic. Output callbacks add
`O(z)` work and constant auxiliary space. Count-per-pattern scans add `O(V+k)`
time and memory: visit counts propagate in reverse BFS order along the failure
tree, collecting exactly those endpoints at which each prefix is a suffix.
Initial root visit 1 accounts for the empty prefix before consuming any byte.

Count-per-position uses `O(n)` result memory; total counting uses constant
workspace. `suffixAggregate` uses `O(V)` combine calls and result storage; BFS
order gives an already-completed suffix fold for every state. Its actual cost
includes caller value copying/combining (for example, concatenated strings may
have larger total output size). Clear releases edge/list contents and retains
top-level vector capacities, including an existing dense table allocation, so
post-clear memory follows prior capacity peaks. A subsequent `build(false)`
releases that dense allocation. Copies cost the full stored representation.

## Feature-to-test map and evidence

The runnable entry is `96-Local Testing/07-Strings/06-aho_tester.py`. Quick/full/
stress enumerate all subsets of binary patterns through length 1/2/2 and all
binary texts through length 3/4/5; they also use ordered dictionaries `[a,b,a]`
for all candidate pairs. These are accompanied by 100/1200/6000 reproducible
random dictionaries and two texts each. Every mode covers every query and both
transition representations. Full/stress enable AddressSanitizer/UBSan.

| Feature | Independent checks |
|---|---|
| Trie/failure/output links, terminal IDs | Reconstruct every state's byte string; directly find its longest proper prefix-state suffix and nearest terminal suffix. |
| Sparse/dense/unknown-byte transitions | Direct longest-suffix comparison for arbitrary states, including NUL, 128 and 255; repeated dense/sparse rebuilds. |
| All counts, empty patterns, duplicates | Direct matching of each pattern at each of all `n+1` boundaries, plus all-prefix occurrence scans. |
| Enumeration and streaming | Directly sorted longest-first output lists, exact callback spans, cancellation at first/middle/last match; nested const callback queries. |
| Generic suffix fold | Direct suffix enumeration with signed addition and noncommutative string concatenation. |
| Forbidden automata | Every arbitrary-state transition and rejecting sink; naive substring tests; independent binary-word enumeration versus automaton DP. |
| Alphabet/lifecycle | All 256 single-byte patterns, NUL/255 pairs, constructor/incremental add, copy/move/clear/assignment and repeated queries/builds. |
| Large output avoidance | Nested prefixes through length 100/1000/2500 and unary text length 10000/150000/600000; closed-form counts, then a final mismatch forcing a long sparse fallback. |
| Preconditions | Seven checked-build death probes: unbuilt query/sink, insertion after build, negative/large states, invalid safe state and mismatched aggregate size. Allocation-boundary limits remain documented preconditions. |

Verification on 2026-09-28:

- `python3 '96-Local Testing/07-Strings/06-aho_tester.py' --mode full --seed 20260928`: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and ASan/UBSan builds all pass **2,267,528 checks per configuration**. Seven checked-build assertion probes pass. The initial sandboxed sanitizer execution hit LeakSanitizer's ptrace restriction; the complete rerun with approved execution outside the sandbox passes without disabling leak detection.
- `python3 '96-Local Testing/07-Strings/06-aho_tester.py' --mode stress --seed 20260929 --configuration optimized`: passes 9,612,363 checks before the final dense-storage-release regression assertion was added; the final full run includes that regression.

## Measured sparse/dense tradeoff

Run `python3 '96-Local Testing/07-Strings/06-aho_benchmark.py' --seed 20260928`.
The adjacent `06-aho_benchmark.jsonl` records complete conditions/results: Intel
Core i9-11900H, GCC 16.2.1, GNU++20 `-O2 -DNDEBUG`, Python 3.14.7, one warmup and
five measured repetitions with medians. Construction includes insertion and
build; scanning calls `countMatches`. Random distributions are uniform in the
reported byte alphabet; nested patterns are unary prefixes. Expected match
totals use direct `string::find` searches or the unary closed form, outside the
timed region. Results are shared-host observations, not timing gates.

| Workload | States / alphabet | Sparse build + scan ms | Dense build + scan ms | Extra dense table bytes |
|---|---|---|---|---|
| 20 patterns of length 5; text 10,000 | 95 / 26 | 0.012 + 0.293 | 0.008 + 0.032 | 9,880 |
| 500 patterns of length 12; text 500,000 | 4,159 / 4 | 0.670 + 18.289 | 0.332 + 2.014 | 66,544 |
| 500 patterns of length 12; text 500,000 | 5,379 / 26 | 0.930 + 26.253 | 0.616 + 2.941 | 559,416 |
| 2,000 patterns of length 20; text 500,000 | 38,221 / 256 | 6.509 + 45.505 | 12.825 + 4.491 | 39,138,304 |
| Unary lengths 1–500; text 300,000 | 501 / 1 | 0.915 + 3.267 | 0.921 + 1.086 | 2,004 |

The last workload counts 149,875,250 matches without materializing them. Dense
storage improves these scans, but doubles construction time and adds about
37.3 MiB in the full-byte case. It remains an explicit choice rather than a
memory-expanding default. Table bytes are measured allocation sizes, not total
RSS: both variants retain the sparse maps, nodes, terminal IDs and build order.

## Sources inspected 2026-09-28

The implementation is independently written; no source code was copied.

- [KACTL, `AhoCorasick.h`, Simon Lindholm, 2015-02-18](https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/AhoCorasick.h): read the entire CC0 header. Compared completed fixed-alphabet transitions, duplicate output chains, and aggregate match counts. That reference excludes empty patterns and assumes uppercase alphabet size 26; this API handles empty patterns and the full byte alphabet explicitly.
- [cp-algorithms, “Aho-Corasick algorithm”](https://cp-algorithms.com/string/aho_corasick.html), also inspected its [Markdown source](https://github.com/cp-algorithms/cp-algorithms/blob/master/src/string/aho_corasick.md): read trie/automaton construction, sparse-map cost discussion, BFS/persistent-transition alternative, output links, aggregate counts and forbidden-word applications. The implemented dense table is alphabet-compressed; sparse maps are the compact alternative. Persistent transition arrays are unnecessary for this bounded-byte scope and no large-integer-alphabet bound is claimed.
- [OI Wiki, “AC 自动机”](https://oi-wiki.org/string/ac-automaton/), inspected [source sections](https://github.com/OI-wiki/OI-wiki/blob/master/docs/string/ac-automaton.md) on failure pointers, BFS completion and “拓扑排序优化”: verified failure-tree occurrence propagation as the count-per-pattern method. The destructive mark-once query shown earlier on that page has different semantics and was not adopted.

The retrieved source SHA-256 values are respectively
`1543e0c7808f82cc564c13fccc9815b675c116eb72d8ca883d187af6bd078d9c`,
`f3faff6e1d95169f4e55ea5e51299f3bf2d7263e61d37b704ca0dafd878de333`,
`19413ac5a19f32c71808cf420ba8e4e5246bb40ef153c1e667feb12ae43297b2`.
These identify inspected retrievals, not immutable upstream commits. Keyword
searches of the preserved `OLD` C++ headers/monoliths found no Aho implementation
to migrate. Legacy bytes were unchanged. No online submission was made.
