# 18-twosat.hpp — Evidence

Owned by package P043 / GR07 (with `21-matching_bipartite.hpp`, `23-assignment.hpp` and `24-dominatortree.hpp`); omissions are in [00-notes.md](00-notes.md#p043-omissions). Contest profile: iterative Tarjan on the implication graph, no recursion, no ISA code. The header replaces the `OLD/algorithms.cpp:4641-4700` excerpt (recursive Kosaraju); its `ans` field stays as the data behind `answer()`, and its `addClause(a, na, b, nb)` and `solve()` map to the literal API (Legacy Mapping).

## Contracts

### TwoSat

`TwoSat(n)` creates variables `[0, n)` and asserts `n >= 0`. A literal is an `int`: `x >= 0` means "variable x is true" and `~x` means "variable x is false". Valid literals lie in `[-n, n)` and are asserted on entry; under `NDEBUG` an out-of-range clause is dropped, so later calls stay in bounds. `clauses` stores every clause `(a, b)`, meaning `a ∨ b`, in insertion order, including the auxiliary clauses of `addAtMostOne`. At most `INT_MAX / 2` clauses are allowed (asserted), so arc IDs fit `int`.

- `addVar()` appends a variable and returns its index; `n` grows and earlier literals stay valid.
- `addClause(a, b)` adds `a ∨ b`; `a == b` is a unit clause.
- `setValue(a)` forces literal `a` true (unit clause). `addImplication(a, b)` adds `a → b`, `addXor(a, b)` adds "exactly one of `a`, `b`", and `addEquivalent(a, b)` adds `a ↔ b`.
- `addAtMostOne(lits)` allows at most one listed literal to be true, counting multiplicity: a literal listed twice is forced false, and `x` with `~x` is always satisfied. For `k = lits.size() >= 2` it adds `k - 2` auxiliary variables `p_i` ("some of `lits[0..i]` is true", `p_0 = lits[0]`) and stores `3k - 5` clauses in O(k) time and memory: `p_{i-1} → ¬lits[i]` for `i ≥ 1`, and `lits[i] → p_i`, `p_{i-1} → p_i` for `1 ≤ i ≤ k - 2`. Every satisfying assignment of the original constraints extends to the auxiliaries (set `p_i` to the prefix OR), and every solution of the encoding satisfies the constraint, so the projection to the original variables is exact.
- `satisfiable()` rebuilds the implication graph as CSR (`start`, `adj`). Clause `i = (a, b)` yields arc `2i: ¬a → b` and arc `2i+1: ¬b → a`, each stored as `(target node, arc ID)`; node `2x` is "x true" and `2x+1` is "x false". It runs iterative Tarjan, stores `comp` (component per node, IDs in sink-to-source emission order) and fills `ans[x] = comp[2x] < comp[2x+1]`. It returns false, and clears `ans`, iff some `x` and `~x` share a component. O(n + m) time and memory.
- `answer()` returns `ans` from the last call; it is stale after later additions until `satisfiable()` runs again. The struct is copyable, and copies are independent.
- Correctness (Aspvall–Plass–Tarjan): an SCC containing both `x` and `¬x` forces a contradiction. Otherwise, setting a literal true iff its component is emitted before its complement's never makes an implication `l → l'` go from true to false: skew symmetry maps the SCC of `l` to the SCC of `¬l` with reversed order, so if `l` is true then `l'`, emitted no later than `l`, is true.

### forcedLiterals

`forcedLiterals()` returns, sorted by variable, every literal that is true in all satisfying assignments (the backbone), auxiliary variables included. It asserts satisfiability; under `NDEBUG` it returns an empty list for an unsatisfiable formula. A literal `l` is false in every solution iff `l ⇝ ¬l`. Such literals are false in the canonical assignment, so only false-side components are candidates. For candidate components in chunks of 64, one pass in topological order (decreasing Tarjan ID) ORs 64-bit reachability masks along the arcs; component `c` fails iff its bit reaches the component of `¬c`, and then the complement of every literal in `c` is forced. Time O((n + m) · (1 + n / 64)), memory O(n + m).

Optimality: no linear algorithm is expected. Split a graph's vertices into A and B, give each vertex a variable, and add a clause `¬a ∨ b` per edge between `a ∈ A` and `b ∈ B` and `¬u ∨ ¬w` per edge inside B. Then `a` is forced false iff two of its neighbours in B are adjacent, that is, iff `a` lies on a triangle. The backbone therefore decides per-vertex triangle membership, for which no linear-time algorithm is known; word-parallel reachability is the practical bound.

### TwoSatWitness and unsatWitness

`unsatWitness()` returns an empty witness when the formula is satisfiable. Otherwise it takes the smallest variable `x` with `x` and `¬x` in one SCC and BFS-shortest implication paths `x ⇝ ¬x` and `¬x ⇝ x`. `literals` is the closed walk `x, …, ~x, …, x`, and `clauses[i]` is the index into `clauses` of a clause `(a, b)` with `{a, b} = {¬literals[i], literals[i+1]}`, so each step is a valid implication. The two paths form an independently checkable refutation. O(n + m) time and memory.

### Legacy Mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `addClause(a, na, b, nb)` (negation flags) | `addClause(na ? ~a : a, nb ? ~b : b)` |
| `solve()` | `satisfiable()` |
| `addExactlyOneOfTwo` (row name) | `addXor(a, b)` |

## Feature-to-Test Map

`96-Local Testing/04-Graphs/18-twosat_tester.py` runs on the shared contest runner with `-Wall -Wextra -Wconversion -Werror`: optimized `-O2 -DNDEBUG`; checked `-g -D_GLIBCXX_DEBUG` (`-O0` in quick, `-O1` in full and stress) with assertion probes; ASan/UBSan `-O1` in full and stress. The oracle enumerates all assignments of the user variables against semantic predicates (clause, implication, xor, equivalence, unit, at-most-one count) that never use the clause encoding. When at most 16 variables exist in total, it also enumerates full assignments of the clause list, auxiliaries included.

| Operation | Test | Oracle |
|---|---|---|
| `addClause`, `setValue`, `addImplication`, `addXor`, `addEquivalent` | Every clause set over 1–2 variables (3 in stress); 6,000 random mixed formulas (full) | Semantic predicates by enumeration |
| `addVar`, `addAtMostOne` | Repeated and complementary literals, k = 0..n+2; 1,000,000-literal list in stress | Projection of encoded solutions equals the semantic solution set; `n` grows by k − 2 |
| `satisfiable`, `answer`, `ans` | All cases; repeated `satisfiable`, re-solve after additions, copies | Existence by enumeration; answer satisfies every clause and predicate; hand-checked re-solve |
| `forcedLiterals` | All satisfiable cases; 20,000-variable chain with forced prefix | Intersection of all solutions (full variable set when ≤ 16, user variables otherwise) |
| `TwoSatWitness`, `unsatWitness` | All unsatisfiable cases; 400,000-variable closed chain | Every step checked against its clause; walk closes at `x` and passes `~x` |
| Preconditions | 5 probes: literal above and below range, negative size, `forcedLiterals` on unsatisfiable input, at-most-one literal out of range | Expected assertion failure |

Five planted mutants on temporary copies, run against the quick suite, were all killed: flipped assignment polarity, flipped forced polarity, dropped prefix implication, unreversed witness clauses, skipped mask propagation.

## Commands and Results

Machine: Intel Core i9-11900H, GNU++20, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), Python 3.14.7. The 2026-11-10 runs used seed 42, beside the other suites, with the build cache emptied before each cold run. The 2026-10-13 runs used the optimized, checked (`-O0` with probes) and ASan/UBSan (`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all`, full and stress) builds, one suite after another, so their times are indicative.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 | `01-run.py --mode full` (P274, library-wide) | PASS (counts not recorded): pre-flight quick peak 760 MB; optimized 0.1 s compile + 0.6 s run, checked 11.3 + 5.1 s, ASan-UBSan 10.9 + 9.6 s; MEMORY peak 924 MB |
| 2026-11-10 | `CXX=g++-14 01-run.py --mode full` (P274) | PASS (counts not recorded): pre-flight quick peak 793 MB; optimized 0.2 + 0.8 s, checked 20.1 + 7.2 s, ASan-UBSan 19.9 + 12.8 s; MEMORY peak 1009 MB |
| 2026-11-10 | `01-run.py --mode quick` (P274) | PASS (counts not recorded): optimized 20.3 + 0.1 s, checked 18.7 + 3.0 s; MEMORY peak 764 MB |
| 2026-11-10 | `01-run.py --mode stress --rounds 1` (P274) | PASS (counts not recorded): pre-flight quick peak 765 MB; optimized 0.1 + 8.8 s, checked 6.2 + 48.9 s, ASan-UBSan 6.5 + 148.4 s; MEMORY peak 1060 MB |
| 2026-10-13 | `18-twosat_tester.py --mode quick` | PASS 2 configurations, 1,673 cases, 42,289 checks, 5 probes |
| 2026-10-13 | `18-twosat_tester.py --mode full --seed 20261013` | PASS 3 configurations, 8,833 cases, 526,625 checks each, 5 probes; 33 s; MEMORY peak 447 MB |
| 2026-10-13 | `CXX=g++-14 18-twosat_tester.py --mode full --seed 20261013` | PASS 3 configurations, 8,833 cases, 526,625 checks each, 5 probes; 34 s; MEMORY peak 461 MB |
| 2026-10-13 | `18-twosat_tester.py --mode stress --seed 3` | PASS 3 configurations, 2,139,985 cases, 31,541,676 checks each, 5 probes; 291 s; MEMORY peak 921 MB |
| 2026-10-13 | `02-integration.py` | PASS 135 standalone and aggregate headers, scalar and available AVX2 multi-TU, workspace; MEMORY peak 650 MB |
| 2026-10-13 | `03-consistency.py` | No errors |
| 2026-10-13 | `03-consistency.py --braces` on the three P043 headers and testers | No closing-brace, comment-cap or free-function violations |

```bash
python3 '96-Local Testing/04-Graphs/18-twosat_tester.py' --mode quick
python3 '96-Local Testing/04-Graphs/18-twosat_tester.py' --mode full --seed 20261013
CXX=g++-14 python3 '96-Local Testing/04-Graphs/18-twosat_tester.py' --mode full --seed 20261013
python3 '96-Local Testing/04-Graphs/18-twosat_tester.py' --mode stress --seed 3
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

None required: Contest profile, no dispatch threshold, no Barrett or Montgomery reduction. The 1,000,000-variable stress chain and at-most-one list are correctness and stack-safety cases, not timings.

## Sources

Fetched 2026-10-09; the ledger is [00-sources.md](00-sources.md#pages-fetched-on-2026-10-09-p043).

| Source | Inspected claim and application |
|---|---|
| [AtCoder Library, twosat](https://raw.githubusercontent.com/atcoder/ac-library/master/atcoder/twosat.hpp) | `satisfiable`/`answer` interface and SCC-order assignment rule |
| [KACTL, 2sat.h](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/graph/2sat.h) | `~x` literal convention, `addVar`, `setValue`, O(k) at-most-one with prefix variables (re-derived; clause set written independently) |
| [cp-algorithms, 2-SAT](https://cp-algorithms.com/graph/2SAT.html) | Implication graph and contradiction criterion |
| [ei1333, two-satisfiability](https://ei1333.github.io/library/graph/others/two-satisfiability.hpp), [Nyaan, two-sat](https://nyaannyaan.github.io/library/math/two-sat.hpp), [maspypy, twosat](https://maspypy.github.io/library/graph/twosat.hpp), [suisen, two_sat](https://suisen-cp.github.io/cp-library-cpp/library/algorithm/two_sat.hpp) | Catalog comparison: unit and set operations present; no backbone or witness API in any |

Legacy: `OLD/algorithms.cpp:4641-4700` (recursive Kosaraju with `addClause(a, na, b, nb)`, `solve`, `ans`). The `TwoSat` structs at `OLD/Team Notebook/src/algsbetter.cpp:1528` and `test.cpp:351` were inspected and have the same interface. Their negation flags map to literals (Legacy Mapping), and their `ans` polarity is preserved (`comp[x] > comp[¬x]` in source-first order equals our sink-first `<`); their recursion and the public `adj`/`adj_t`/`used`/`ord` fields are not.

## Limits and Handoffs

- The row is `partial`: `TwoSat::addNand` is missing.
- Incremental 2-SAT, the lexicographically smallest assignment, solution counting (#P-hard) and Horn-SAT stay out of this row ([00-notes.md](00-notes.md#p043-omissions)).
- The Python twin `08-Python/_33_twosat.py` is a separate row.
- There is no mutation record yet; the planted mutants are the only mutation evidence.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): 2-SAT; 二元可满足性.

## History

- 2026-10-09: verified (P043); quick, full and stress passed on g++, full on g++-14; independent review: no wrong answers (own brute-force 2-SAT oracle, overflow probes at the stated domain bounds).
- 2026-10-13: prune removed `addClause(a, na, b, nb)` and `solve()` (Legacy Mapping); the two call styles in the tests collapsed into the literal form and the legacy section became a re-solve and copy check; full passed on g++ and g++-14, stress on g++.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
