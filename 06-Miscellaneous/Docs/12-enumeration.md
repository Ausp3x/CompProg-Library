# 12-enumeration.hpp — evidence

`12-enumeration.hpp` (batch MI15, package P015, status audit) provides visitor-based traversals of combinations, multicombinations, subsets, mixed-radix products and Gray products, and of submasks, subset masks, fixed-popcount masks and Gray masks. It reuses verified MI02 `BitOps` and the C01 template (P002). Implementations are independent contest-profile code. No online submission was made.

## Contracts

### Traversals (`forEachCombination`, `forEachMulticombination`, `forEachSubset`, `forEachProduct`, `forEachGrayProduct`, `forEachSubmask`, `forEachSubsetMask`, `forEachCombinationMask`, `forEachGrayMask`)

Every traversal takes a visitor returning `true` to continue. A `false` visitor result stops immediately, including at the final output, and makes the traversal return `false`; `true` means normal completion. Vector arguments are borrowed immutable state valid only during the callback. Copy to retain an output, and do not mutate a radix vector through another alias while traversing it. Exceptions propagate; no global state survives a call.

| API | Order, domain and boundary semantics |
|---|---|
| `forEachCombination(n,k,visit)` | Nonnegative `int` dimensions. Increasing index vectors in lexicographic order; `k>n` emits nothing, `k=0` emits one empty vector. Repeated input values remain distinct positions. |
| `forEachMulticombination(n,k,visit)` | Nondecreasing lexicographic index vectors with replacement; `n=0<k` emits nothing, and `k=0` emits one empty vector. Avoids forming an overflowing `n+k-1`. |
| `forEachSubset(n,visit)` | Nonnegative `int n`, no machine-word limit. Binary subset order with selected indices stored in **decreasing** order. The empty set comes first; `n=0` emits it once. |
| `forEachProduct(radices,visit)` | Nonnegative radices, at most `INT_MAX` dimensions; last digit fastest. Any zero radix gives no output; no dimensions gives one empty tuple. |
| `forEachGrayProduct(radices,visit)` | Same domain, reflected mixed-radix order. Reports the changed digit, initially `-1`; consecutive outputs change exactly one digit by one. The traversal is not promised cyclic. |
| `forEachSubmask(mask,visit)` | Descending submasks including the mask and zero, using canonical `BitOps::prevSubmask`. |
| `forEachSubsetMask<U>(n,visit)` | All low-`n` masks in numerical order. |
| `forEachCombinationMask<U>(n,k,visit)` | Fixed-popcount low-`n` masks in numerical/colex order, using canonical `BitOps::nextCombination`; this differs from vector-combination lexicographic order. `k>n` emits nothing. |
| `forEachGrayMask<U>(n,visit)` | Binary reflected Gray masks with changed-bit index, initially `-1`. |

Mask APIs support the unsigned 8–128-bit word domains of `BitOps`, with `0<=n<=word width` and nonnegative `k`. They test the last index before incrementing, so full-width enumeration requires no unrepresentable `2^width` counter or shift. Early stopping makes prefixes of otherwise enormous traversals practical. These APIs intentionally do not return a potentially overflowing total output count; ranking/unranking remains owned by permutation/combinatorics APIs.

Correctness: lexicographic combinations increment the rightmost movable index and restore the smallest increasing suffix. The subset vector acts as a binary counter: trailing low selected indices are removed and the first absent index is appended. Product carries and Gray-direction reversals skip unit radices; for radices at least two, carries/reversals have geometrically decreasing frequencies, giving amortized constant overhead per output after linear setup. Gray masks toggle the trailing-zero bit of the new binary index, equivalent to `i ^ (i >> 1)`.

Costs: vector combinations use `O(k)` state and worst-case `O(k+1)` delay; subsets `O(n)` state and amortized constant delay; products `O(d)` state/setup and amortized constant delay; masks constant state and delay. Visitor work and retained output copies are additional. No recursion or precomputed exponential output list is used.

## Feature-to-test map

The mirrored `12-enumeration_tester.py` entry uses independent recursive combination/subset/product references, recursive reflected Gray lists, and independently counted mask bits. Non-removable checks run in optimized NDEBUG, checked and sanitizer builds.

| Public feature | Coverage |
|---|---|
| Combinations and multicombinations | Exhaustive small dimensions, positional order, `k>n`, empty identities, `INT_MAX` prefixes and large repeated-index output. |
| Arbitrary-dimensional subsets | Independent recursive binary-order sets, decreasing-index invariant, empty set, 100,000-dimensional prefixes. |
| Product and Gray product | Exhaustive radix vectors of length 0–5 with radices 0–3; 1,500 seeded random products; zero/unit radices; 100,000-dimensional sparse products; changed-digit and adjacency checks. |
| Four mask traversals | Unsigned 8/16/32/64/128-bit instantiations, exhaustive small words, random sparse masks, full-width masks and boundary combinations, changed-bit checks. |
| Visitor/lifetime behavior | Immutable vector type checks, cancellation at first/intermediate/final output, saved copies, repeated and nested calls, exceptions from every API. |
| Preconditions | 16 checked-build assertion probes, including a negative radix after a zero radix. |

## Commands and results

Run 2026-09-28: Linux x86-64, i9-11900H, GCC 16.2.1 (20260810), CPython 3.14.7. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie` with leak checking enabled.

```bash
python3 '96-Local Testing/06-Miscellaneous/12-enumeration_tester.py' --mode full --seed 20260928                               # PASS, optimized and checked
python3 '96-Local Testing/06-Miscellaneous/12-enumeration_tester.py' --mode full --seed 20260928 --configuration ASan-UBSan    # PASS
python3 '96-Local Testing/01-run.py' --mode quick --seed 20260928 --filter 12-enumeration --no-integration                    # PASS, also when run from /tmp
python3 '96-Local Testing/02-integration.py'                                                                                   # PASS, 93 headers, scalar/AVX2 multi-TU, workspace
python3 '96-Local Testing/03-consistency.py'                                                                                   # no errors
```

Stress mode exists but was not run. No `CXX=g++-14` floor run is recorded.

## Benchmarks

None: the direct successors have no dispatch, cutoff or speed claim; bounds follow from the carry/successor arguments above.

## Sources

Inspected on 2026-09-28; these are algorithm/proof references, and the implementation was independently written without copying external source:

- [CP-algorithms combinations](https://cp-algorithms.com/combinatorics/generating_combinations.html): lexicographic successor and fixed-weight Gray recursion; the latter is separate from this header's ordinary binary/mixed-radix Gray traversals.
- [CP-algorithms submasks](https://cp-algorithms.com/algebra/all-submasks.html): descending successor proof and zero-termination hazard.
- [CP-algorithms Gray code](https://cp-algorithms.com/algebra/gray-code.html): reflected formula, inverse and adjacency.
- [Python itertools](https://docs.python.org/3/library/itertools.html): `combinations` and `product` API descriptions/reference algorithms, positional identity and empty-product semantics.
- Jörg Arndt, [*Matters Computational*](https://www.jjj.de/fxt/fxtbook.pdf), preface June 2010, chapter 9 pp.217–223: odometer carry analysis, constant-amortized reflected traversal and loopless focus-pointer alternative. FXT's GPL code was not adapted.
- [OI Wiki Gray code](https://oi-wiki.org/math/numeral-sys/gray-code/): construction/proof/inverse, partly translated from CP-algorithms and therefore not independent corroboration.
- Torsten Mütze, [*Combinatorial Gray codes—an updated survey*](https://arxiv.org/html/2202.01280v4), revision 2024-07-30, §§2.5, 3.19, 4.1, 4.2: amortized versus worst-case delay, ranking/unranking, larger alphabets, revolving-door and cool-lex combinations. These selected sections were read; this is not a claim of a complete paper survey.

## Limits and handoffs

- Open `/reaudit-review` findings for P015 ([p015.md](<../../00-Guidelines/23-Reaudit Findings/p015.md>)), not yet resolved:
  - 1: the inventory row assigns `forEachIntegerPartition`, `forEachSetPartition`, `combinationRank` and `combinationUnrank` to this header; none is implemented, so the row stays partial.
  - 2: the contract statement that ranking/unranking is owned by the permutation/combinatorics APIs, and the P015 completion claim in [00-notes.md](00-notes.md), contradict that row.
  - 3: seven function bodies end with a lone `}` (closing-brace rule).
  - 4: complexity comments use undefined symbols `outputs` and, in `forEachGrayProduct`, `d`.
- No enumeration implementation was found in the legacy index or `OLD`. Fixed-weight Gray/cool-lex orders, loopless worst-case-delay generators, combination ranking/sampling and restricted combinatorial objects are future extensions. Permutation ranking remains in MI02; constrained search keeps its own owner.
- Stress mode and other compiler/interpreter versions (including the GCC 14 floor) were not run.
