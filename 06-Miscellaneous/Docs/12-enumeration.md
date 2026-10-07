# 12-enumeration.hpp — evidence

`12-enumeration.hpp` (batch MI15) provides visitor-based traversals of combinations, multicombinations, subsets, mixed-radix products and Gray products, and of submasks, subset masks, fixed-popcount masks and Gray masks. Package P015 (order MI15, MI16, MI17). The verified C01 template prerequisite is supplied by P002. Implementations are independent contest-profile code. Archived originals remain unchanged. No online submissions are part of this work. Enumeration also reuses verified MI02 `BitOps`; this dependency on canonical MI02 bit stepping is reflected in the batch table, batch map and P015 prerequisite package map.

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

## Correctness and costs

Lexicographic combinations increment the rightmost movable index and restore the smallest increasing suffix. The subset vector acts as a binary counter: trailing low selected indices are removed and the first absent index is appended. Product carries and Gray-direction reversals skip unit radices. For active radices at least two, carries/reversals have geometrically decreasing frequencies, giving amortized constant overhead per output after linear setup. Gray masks toggle the trailing-zero bit of the new binary index, equivalent to `i ^ (i >> 1)`.

Vector combinations use `O(k)` state and worst-case `O(k+1)` delay. Subsets use `O(n)` state with amortized constant delay. Products use `O(d)` state/setup and amortized constant delay, and masks use constant state/delay. Visitor work and retained output copies are additional. No recursion or precomputed exponential output list is used.

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

```bash
python3 '96-Local Testing/06-Miscellaneous/12-enumeration_tester.py' --mode full --seed 20260928
python3 '96-Local Testing/06-Miscellaneous/12-enumeration_tester.py' --mode full --seed 20260928 --configuration ASan-UBSan
```

Stress mode is available but was not executed.

Final environment: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 (20260810), CPython 3.14.7. Full configurations use GNU++20 optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie`. Leak checking remained enabled. The initial sanitizer failures came from LeakSanitizer's documented inability to run beneath the sandbox process tracer; approved runs outside that tracer passed. No algorithm failures remained.

Package-wide P015 integration and consistency commands passed:

```bash
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

Integration compiled all 93 present standalone/aggregate headers, linked combined scalar and available AVX2 aggregates across two translation units, and checked the standalone Workspace in LOCAL/non-LOCAL modes. Repository consistency reported no errors. Every saved benchmark source hash matched the final source bytes.

The shared runner discovered and passed this suite from `/tmp`, confirming that tests resolve paths independently of the caller's directory:

```bash
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --seed 20260928 --filter 12-enumeration --no-integration
```

## Benchmarks

The canonical direct successors have no hardware dispatch, tuning cutoff or claimed speed superiority requiring a timing comparison. Bounds follow from the carry/successor arguments above.

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

No enumeration implementation was found in the targeted local legacy index/OLD search. Fixed-weight Gray/cool-lex orders, loopless worst-case-delay generators, combination ranking/sampling and restricted combinatorial objects are explicitly future extensions, not implied by ordinary traversal completion. Permutation ranking remains in MI02, and constrained search retains its separate owner.

Specialist enumeration orders/ranking need their own scoped extension; they are not claimed implemented by P015. Extended stress mode and execution on other compiler/interpreter versions were not performed. No external online acceptance is claimed, and no online submission was made.
