# 06-aho.hpp — evidence

`06-aho.hpp` implements the static multi-pattern Aho-Corasick row in full, including `fromTrie` over an external labelled tree. The dictionary accepts all bytes, including NUL and bytes 128–255, duplicate pattern IDs and empty patterns. Dynamic dictionary changes belong to the separately planned `34-dynamicaho.hpp`.

## Contracts

### AhoCorasick

Symbols: `L` total inserted pattern length, `k` number of IDs, `V` number of states, `S <= 256` number of distinct bytes on trie edges, `n` text length, `out` number of reported matches, `h` depth of a state.

- Construction: `AhoCorasick ac(patterns, dense)` builds a ready dictionary (`dense` defaults to false). Alternatively call `add(pattern)` on a fresh or cleared object, which returns the insertion-order ID, then `build(dense)`. `add` after `build` violates a precondition. Repeating `build` rebuilds or switches the representation and keeps IDs and states; switching to sparse releases the dense table. `clear` restarts IDs and states and keeps top-level vector capacities, including an existing dense table, until a later `build(false)`. Copies own all storage; a moved-from object must be cleared or assigned before other operations. Pattern and text views need to stay valid only during the call. Allocation failure follows the standard containers; no transactional recovery is promised.
- Domain: every pattern and text length, the number of IDs and the number of states are below `INT_MAX`. Per-boundary counts are `int` (at most `k`); per-pattern, per-state and total counts are `lng`, and `(n+1) * k` fits in signed 64 bits under these limits. Root is state 0 and recognizes the empty prefix; an empty pattern matches at all `n+1` boundaries. Duplicate patterns get separate IDs and each contributes to counts.
- `fromTrie(parent, label, ends = {}, dense = false)` builds the automaton over an external trie with `N = parent.size()` nodes. Node 0 is the root and `parent[0]`, `label[0]` are ignored. Every other node `v` hangs below `parent[v]` by byte `label[v]`; node ids may be in any order. Preconditions (asserted): `label.size() == N`, parents in range, distinct labels among siblings, and every node reachable from the root (a cycle is detected because the BFS order then misses nodes). State `v` of the result is node `v`. `ends[i]` (repeats allowed) becomes pattern ID `i` with `length[i]` equal to the node depth; with no `ends` the automaton has no IDs and only links, transitions and `suffixAggregate` are meaningful. All queries then behave exactly as for an automaton built by `add` from the node strings.
- Read-only fields: `nodes[u].next` (trie children), `nodes[u].ids` (IDs ending exactly at `u`), `nodes[u].link` (longest proper suffix that is a state; the root links to itself), `nodes[u].out` (nearest proper suffix state with an ID, `-1` if none), `nodes[u].count` (`matchCount`), `terminal[id]`, `length[id]`, and `order` (BFS order, a topological order of the failure tree). `go`, `code`, `sigma`, `built`, `dense` and the helpers `walk`, `next`, `outputs`, `checkState` are internal and unchecked.
- `step(u, c)`: the longest state that is a suffix of `str(u) + c`; bytes absent from every pattern return the root in `O(1)`. Keep the state across text chunks for streaming. `sparseStep(u, c)` is the same transition computed by the failure walk, valid in both representations. Both assert `built` and `0 <= u < size()`. A single sparse transition costs `O((h+1) * log(S+2))`; a scan from the root amortizes to `O(log(S+2))` per byte. Dense transitions cost `O(1)`.
- `matchCount(u)`: number of IDs ending at `u`, including every terminal suffix and empty IDs. `forbidden(u)` is `matchCount(u) != 0`. `safeStep(u, c)` returns `-1` (an absorbing rejecting sink) if `u` is already forbidden or the next state is; `safeStep(-1, c)` stays `-1`. `avoids(text)` reports whether no pattern occurs in `text`; empty patterns forbid every text.
- Forbidden-language counting: start with `dp[0] = !ac.forbidden(0)`; for each position, state and allowed byte add `dp[u]` to the next layer at `ac.safeStep(u, c)` when it is nonnegative. Any byte subset is allowed, including bytes absent from the patterns. Dense builds make arbitrary DP transitions `O(1)`.
- `suffixAggregate(value, combine)`: returns `value[u]` folded with every proper suffix state, nearest first, the root once. `combine` must be associative and may be noncommutative; the caller handles arithmetic safety and value copies. `O(V)` combine calls.
- `forEachOutput(u, visit)`: calls `visit(id) -> bool` for the IDs ending at `u`, longest pattern first, duplicates in ID order, in `O(out + 1)`. `forEachMatch(text, visit)`: calls `visit(id, l, r) -> bool` for every match `[l, r)` by increasing end boundary `r`, each boundary in output order, with constant workspace and no stored match list. False from a callback cancels and the call returns false, even at the last match. Callbacks may run nested const queries but must not mutate the automaton. For streaming, report the root's outputs once before the first chunk, then after each consumed byte.
- `countPatterns(text)`: one count per ID. `countPositions(text)`: `n+1` counts indexed by end boundary. `countMatches(text)`: total match count. `stateCounts(text)`: occurrences of every state's string as a substring ending at a boundary, root `n+1`. None materializes outputs.

Complexity: sparse construction `O(k + L * log(S+2))` time and `O(V + k)` memory; dense construction adds `O(V * S)` time and exactly `4 * V * S` bytes of completed transitions (only used bytes get columns). `add` costs `O(m * log(S+2))` for a pattern of length `m`. Scans cost `O(n * log(S+2))` sparse and `O(n)` dense; counts per pattern or state add `O(V + k)`, positions add `O(n)` result memory, callbacks add `O(out)`. For `fromTrie`, the sparse failure walk is bounded by `D`, the sum of leaf depths of the external trie, which can reach `Theta(N^2)` (an adversarial trie with `N = 60,003` took 683 ms with the walk against 2 ms dense). `fromTrie` therefore computes `D` in its own BFS (needed anyway for node depths and the reachability check) and, when `D > N * S`, builds the links through a temporary dense table and releases it before returning a sparse automaton. Sparse `fromTrie` costs `O(k + min(D * log(S+2), N * S))` time with `O(N + k)` retained memory; its peak is `O(N * S)` only when `D > N * S`. A later explicit `build(false)` on such an object walks again and is bounded by `D`. maspypy's persistent-array alternative (`O(N * log(S))`) was not adopted: the byte alphabet bounds `S <= 256`, the threshold keeps the build within `O(N * S)`, and the persistent structure would triple the code.

Correctness: the state invariant is the longest pattern prefix that is a suffix of the processed text. Failure links point to the longest proper such suffix; BFS processes every shallower state first, so the parent's failure state already has final transitions (dense) or links (sparse walk). Output links skip to the nearest terminal suffix, so following them enumerates each matching ID once, longest first, including the root's empty IDs. `count[u] = |ids[u]| + count[link[u]]` follows. The sparse build is bounded by telescoping: along each root-to-leaf path the failure depth rises by at most one per edge and every failed walk step lowers it, so the walk total is at most the summed leaf depths (`<= L` for added patterns). Per-state occurrence counts propagate in reverse BFS order along the failure tree, crediting each end boundary to every suffix state.

## Feature-to-test map

Runnable entry: [06-aho_tester.py](<../../96-Local Testing/07-Strings/06-aho_tester.py>), suite [06-aho_tester.cpp](<../../96-Local Testing/07-Strings/06-aho_tester.cpp>), built with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Quick/full/stress enumerate every subset of binary patterns through length 1/2/2 against all binary texts through length 3/4/5, ordered duplicate dictionaries `[a, b, a]`, 100/1200/6000 seeded random dictionaries (alphabets 1, 3, 256) and 60/600/3000 random external tries. Every mode covers every query in both representations.

| Feature | Independent checks |
|---|---|
| Node, add, size, patterns, trie states | Insertion IDs; state count equals the number of distinct pattern prefixes; every state's string rebuilt from the trie; `terminal`/`length` per ID. |
| build (sparse/dense), link, out, step, sparseStep | Direct longest-suffix search for every state and bytes 0, 97–99, 127, 128, 255, checking `step` and `sparseStep` in both representations; repeated sparse/dense rebuilds; dense release on `build(false)`. |
| fromTrie | Random labelled trees with shuffled node ids, alphabets 2–4 and 256, up to 40 nodes; node strings rebuilt from the parent chain; node ids preserved; sparse results release the table; every structural and text query compared with the direct oracles; single-node trie. Adversarial tries (spines `a^i`, `b a^i`, a `c` leaf below every `b a^i`) with `m = 3` (walk path) and `m = 20` (temporary dense path, `D > N * S`) under the full oracle, and `m = 500/5000/20000` with closed-form links and counts. Caterpillar trie (spine 2000/50000/200000 with a leaf at every spine node) with closed-form links, counts and depths, sparse and dense. |
| matchCount, countPatterns, countPositions, countMatches, stateCounts | Direct matching of every pattern and state string at all `n+1` boundaries; empty and duplicate IDs. |
| forEachOutput, forEachMatch | Sorted longest-first expected lists, exact callback spans, cancellation at first/middle/last match, nested const queries from callbacks. |
| suffixAggregate | Direct suffix enumeration with signed addition and noncommutative string concatenation. |
| forbidden, safeStep, avoids | Every arbitrary-state transition and the rejecting sink; naive substring search; binary-word enumeration versus automaton DP for lengths 0–7. |
| clear, lifecycle, bytes | All 256 single-byte patterns, NUL/255 pairs, constructor versus incremental add, copy/move/clear/assignment and repeated queries. |
| Large counts | Nested unary patterns depth 100/1000/2500 on unary text 10000/150000/600000, closed-form counts, then a final mismatch forcing long sparse fallbacks. |
| Preconditions | 15 checked-build death probes: unbuilt `step`/`sparseStep`/sink, `fromTrie` duplicate sibling label, unreachable cycle, parent out of range, end out of range, label size, `add` after build, negative and past-end states for `step`/`matchCount`/`sparseStep`/`forEachOutput`, invalid safe state, aggregate size. |

Mutation sanity (not a gate): dropping the terminal check in output links, a wrong `fromTrie` depth, an off-by-one failure walk in `sparseStep` and a corrupted link on the temporary dense path each fail the quick suite.

## Commands and results

Run 2026-10-08 with the configurations listed in [01-prefixfunction.md](01-prefixfunction.md#commands-and-results).

```bash
python3 '96-Local Testing/07-Strings/06-aho_tester.py' --mode quick --seed 20261008  # PASS, 2 configurations, 281,056 checks each, 15 assertion probes
python3 '96-Local Testing/07-Strings/06-aho_tester.py' --mode full --seed 20261008  # PASS, 3 configurations, 3,221,100 checks each
CXX=g++-14 python3 '96-Local Testing/07-Strings/06-aho_tester.py' --mode full --seed 20261008  # PASS, 3 configurations, 3,221,100 checks each
python3 '96-Local Testing/07-Strings/06-aho_tester.py' --mode stress --seed 20261009  # PASS, 3 configurations, 14,440,928 checks each
python3 '96-Local Testing/07-Strings/06-aho_benchmark.py' --seed 20261008  # PASS, every workload verified against direct counts
```

The pre-change suite passed in full mode (seed 1, three configurations) as the re-audit baseline. Integration, the folder run and consistency are recorded in [07-suffixarray.md](07-suffixarray.md#commands-and-results).

## Benchmarks

`python3 '96-Local Testing/07-Strings/06-aho_benchmark.py' --seed 20261008`: Intel Core i9-11900H, GCC 16.2.1, GNU++20 `-O2 -DNDEBUG`, Python 3.14.7, one warmup and five measured repetitions, medians. Construction covers insertion (or `fromTrie`) and build; scanning calls `countMatches`, checked against `string::find` counts or the unary closed form outside the timed region. Random texts and patterns are uniform over the stated alphabet. Shared-host observations, not timing gates.

| Workload | States / alphabet | Sparse build + scan ms | Dense build + scan ms | Dense table bytes |
|---|---|---|---|---|
| 20 patterns of length 5; text 10,000 | 93 / 26 | 0.006 + 0.158 | 0.005 + 0.014 | 8,928 |
| 500 patterns of length 12; text 500,000 | 4,151 / 4 | 0.484 + 13.173 | 0.237 + 1.456 | 66,416 |
| 500 patterns of length 12; text 500,000 | 5,372 / 26 | 0.496 + 20.050 | 0.275 + 2.048 | 558,688 |
| 2,000 patterns of length 20; text 500,000 | 38,232 / 256 | 4.920 + 36.407 | 8.728 + 2.884 | 39,149,568 |
| Unary lengths 1–500; text 300,000 | 501 / 1 | 0.549 + 2.013 | 0.549 + 0.644 | 2,004 |
| `fromTrie`: trie of 20,000 patterns of length 12; text 500,000 | 192,128 / 26 | 32.518 + 43.151 | 24.246 + 6.261 | 19,981,312 |
| `fromTrie`: caterpillar spine 200,000; text 500,000 | 400,001 / 2 | 22.368 + 4.832 | 21.915 + 1.253 | 3,200,008 |
| `fromTrie`: adversarial spines 20,000 (`D` about `2 * 10^8`); text 500,000 | 60,003 / 3 | 2.166 + 5.936 | 2.215 + 1.271 | 720,036 |

Before the leaf-depth threshold, the adversarial sparse build took 683.259 ms.

Dense scans are 3–13 times faster; dense builds match or beat sparse ones for small alphabets and are about twice as slow with 256 symbols, where the table costs 37 MiB. Dense stays an explicit choice. The unary workload counts 149,875,250 matches without materializing them.

## Sources

Implementation independently written; no code copied.

- [KACTL `AhoCorasick.h`](https://github.com/kth-competitive-programming/kactl/blob/main/content/strings/AhoCorasick.h) (CC0): completed fixed-alphabet transitions, duplicate output chains, aggregate counts. It excludes empty patterns and assumes 26 uppercase letters; this API handles empty patterns and all bytes.
- [cp-algorithms, Aho-Corasick](https://cp-algorithms.com/string/aho_corasick.html): trie and automaton construction, sparse-map cost, BFS with persistent transitions, output links, counts and forbidden-word applications.
- [OI Wiki, AC 自动机](https://oi-wiki.org/string/ac-automaton/): failure-tree topological propagation for per-pattern counts (adopted); its destructive mark-once query has different semantics and was not adopted.
- [maspypy `aho_corasick_for_general_trie.hpp`](https://github.com/maspypy/library/blob/main/string/aho_corasick_for_general_trie.hpp) (read 2026-10-08): failure links over an arbitrary labelled tree (any vertex order, integer labels, root 0) via persistent arrays, `O(N * log(K))`. `fromTrie` adopts the input model with byte labels and adds output links and IDs; the dense build replaces the persistent arrays (see Contracts).
- [ei1333 `aho-corasick.hpp`](https://ei1333.github.io/library/string/aho-corasick.hpp), [Nyaan](https://nyaannyaan.github.io/library/string/aho-corasick.hpp), [suisen](https://suisen-cp.github.io/cp-library-cpp/library/string/aho_corasick.hpp), [hitonanode](https://hitonanode.github.io/cplib-cpp/string/aho_corasick.hpp), [tko919](https://tko919.github.io/library/), [Library Checker `aho_corasick`](https://github.com/yosupo06/library-checker-problems/tree/master/string/aho_corasick) (read 2026-10-08): completeness sweep; their operations map to `step`/`sparseStep`, `matchCount`, `forEachOutput`, `forEachMatch` and `countPatterns`.

The preserved `OLD` headers contain no Aho-Corasick implementation.

## Limits and handoffs

- `/reaudit-review` findings ([p017](<../../00-Guidelines/23-Reaudit Findings/>), deleted after this run): 1 `sparseStep` now asserts `built` and the state range, and is tested directly against the oracle in both representations; 3 `fromTrie` implemented and tested; 4 closing braces normalized; 5 scans use the unchecked `next` and direct counts after one entry check; 6 complexity comments rewritten in `S/U/Q/M` and `T/M` form with `out`, and the contract text moved here.
- Not adopted: a parent field (Library Checker `aho_corasick` prints parents; recover them in `O(V)` from `nodes[u].next`) and scans resuming from a given state (ei1333 `move`; use `step` and `matchCount` directly).
- ST13 (`34-dynamicaho.hpp`) owns dynamic updates; ST11 owns generalized multiple-string indexes.

## History

- 2026-09-28: verified under the previous system (P017) without `fromTrie`; optimized stress seed 20260929 passed 9,612,363 checks. 2026-10-08 re-audit: `fromTrie` added, `/reaudit-review` findings 1 and 3–6 fixed.
