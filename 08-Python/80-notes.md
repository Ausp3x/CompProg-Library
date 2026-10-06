# 08 Python — notes

Contracts moved out of `00-index.md` on 2026-10-06. Rules here bind every module in the folder; operation lists live in the index.

## Interpreter and baseline

- Python 3.10 syntax and standard library only; must run on CPython 3.10+ and PyPy 3.10+. APIs newer than 3.10 (`math.cbrt`, `itertools.batched`, `Fraction.is_integer`) are not used unconditionally. Available in 3.10 and relied upon: `int.bit_count`, `bisect` `key=`, `itertools.pairwise`, `math.isqrt`/`comb`/`perm`/`lcm`/multi-arg `gcd`, `pow(a, -1, m)`, `functools.cache`.
- PyPy is the primary performance target: lists of ints over tuples, pairs packed into one int when hot, iterative traversal or the generator trampoline (`_51_pypy_idioms.bootstrap`) instead of deep recursion, bytes I/O, no `locals()`/`sys._getframe()` in hot code, string building through `join`. Long integers above 64 bits run in non-JIT support code on PyPy; bounds must count bit length separately from element count.
- A missing interpreter is a coverage gap, never a pass. No module raises the recursion limit, reseeds the global RNG, or performs I/O at import time; `_98_basic.py`/`_99_all.py` only re-export names.
- `_51_pypy_idioms.modMul64` uses `__pypy__.intop` behind an `isPyPy` guard with a plain-`int` fallback that returns identical results; it is the only module allowed to import a PyPy-specific name, and the import is guarded.

## Numeric policy

- Native `int`, `pow`, `divmod`, `math.isqrt`/`gcd`/`lcm`/`comb` and `fractions.Fraction` replace Core Barrett/Montgomery/modint/InfInt/Rational types; no Python wrapper of those types is planned.
- Floor-division and modulo semantics are Python's (floor toward negative infinity); modules that need C truncation (`divmodFloor` versus `extgcd` conventions) say so in the docstring.
- `Fraction(float)` preserves the binary float, not the intended decimal; exact rational inputs are integer pairs or strings. Exact and approximate variants are distinct functions (`lineIntersection` in `_11` versus `lineIntersectionF` in `_41`; `convolveInt` exact versus `convolveFft` bounded).
- Python big-int bit operations are not constant-time machine instructions: `_22`, `_45`, `_56` charge `O(n / 64)` per word-level operation and state the bit width of every mask. Negative bit patterns are rejected or normalized explicitly.
- Deterministic Miller–Rabin in `_14` is proved only below the recorded bound (3.3e24 with the fixed base set); above it `isProbablePrime` is the only honest name. Arbitrary-precision arithmetic does not extend the proof domain.
- `_20_polynomial`: NTT paths require an NTT-friendly prime; `convolveMod` reconstructs through three NTT primes; `convolveInt` packs coefficients into one big int with a documented coefficient bound; FPS inverse/log/exp/sqrt require the characteristic/unit and square-root preconditions named in their docstrings. The crossover between naive and transform convolution is measured and recorded.
- `_39`: XOR/AND/OR inverse transforms stay exact over integers (divisibility by the transform length) or use modular unit inverses; the float `/=` of the reviewed source is never copied. Bitmask transforms require power-of-two length; gcd/lcm transforms require integer indices within the stated bound.
- `_37`: potentials are initialized by Bellman–Ford when any cost is negative; a negative cycle in the initial residual graph is a rejected input. Zero potentials are correct only for nonnegative costs.

## Structural and domain contracts

- Containers: vertices are `0..n-1`; ranges are half-open `[l, r)`; empty-range folds return the identity; `None` or a documented sentinel means "valid input, no answer"; `assert` is reserved for programmer preconditions because `-O` removes it.
- Graph inputs are adjacency lists (`buildAdj`) or CSR arrays (`buildCsr`); multigraphs, self-loops and disconnected graphs are accepted unless a function says otherwise (`_34` edge ids distinguish parallel edges; `_35` reports isolated vertices and empty graphs).
- `_02`: `list.insert` is linear; `bisect`-based structures never claim logarithmic insertion. `_31_ordered_multiset` gives the measured per-operation cost of the bucketed list (amortized sqrt-block) and the treap (expected log); `WordSet` is the integer-key fast path.
- `_13`: sparse-table `O(1)` holds for idempotent operations only; `DisjointSparseTable` is the non-idempotent alternative; empty queries need an explicit identity.
- `_08`/`_29`: noncommutative monoids fold left to right; `maxRight`/`minLeft` predicates must be monotone and accept the identity.
- `_17`: rerooting has one canonical owner here (`reroot`); `_30` calls it rather than duplicating it. Trees are forests when stated; node/edge indexing is zero-based.
- `_18`: rollback DSU uses union by size without path compression; `WeightedDsu` returns `None` for inconsistent merges. Offline dynamic connectivity is the segment-tree-over-time reduction.
- `_19`/`_56`: persistent trees use parallel node arrays with explicit version ids; the wavelet matrix owns its compact `BitVector`; `_45` keeps the sampled, general rank/select variant.
- `_26`: Mo variants state sorting order (block or Hilbert), tie rules and mutation rollback; tuple allocation per query is avoided by packing (`_51.packPair`).
- `_27`: every randomized routine takes a caller-controlled `random.Random`; Monte Carlo and Las Vegas labels are explicit; `_14` owns its own Pollard-rho loop and shares only the RNG policy.
- `_28`: `os.read`/`os.write` paths are benchmarked against buffered `sys.stdin.buffer` before use; interactive programs use `_01.Interactive`, which flushes after every query.
- `_40`: `aStar` reopens closed states under merely admissible heuristics; a no-reopen contract requires a consistent heuristic. Depth/time budgets and duplicate-state policies are parameters, not defaults.
- `_41`: tolerances are scale-aware and documented; Welzl's MEC shuffles with the caller's seed; residual checks accompany every returned intersection.
- `_43`: digit DP, profile DP and interval DP are templates with callbacks; they do not conceal problem-specific state proofs. `_58` owns Sprague–Grundy and retrograde analysis so `_43` game DP is limited to witness reconstruction.
- `_44`, `_45`, `_46`, `_49`, `_50`: low PyPy value (big-int masking, pointer-heavy nodes, exponential search, probabilistic structures rarely needed in contests). They remain Esoteric with explicit resource bounds and are implemented after every Basic and Advanced row.
- `_49` combines SAT (`Dpll`) with exact cover (`DancingLinks`); `_50` combines sketches with numerical integration/roots. Splitting them into separate rows is a future renumbering task; until then each module documents both halves.
- All-solution and enumeration APIs return bounded enumerations or compact parameterizations and are charged by output size.

## Ownership boundaries

- C++ folders own the full families: `02` (DSU, Fenwick, segment trees, sparse tables, ordered sets, wavelet, sqrt decomposition, interval map, persistent and rollback structures), `03` (exact/float geometry, calipers, closest pair, half-plane, Minkowski), `04` (traversal, SCC, lowlink, flows, matchings, trees, dominator tree, Euler trails, functional graphs), `05` (number theory, combinatorics, polynomials, matrices, transforms, Stern–Brocot, Burnside, games), `06` (I/O, randomization, DP families, state search, SAT/exact cover, sketches), `07` (KMP/Z/Manacher, hashing, tries, Aho–Corasick, suffix structures, eertree, Lyndon, BWT/FM-index). A Python row is a documented reduction of its C++ owner.
- Within Python: GF(2) elimination is in `_21`, `XorBasis` in `_22`; meet-in-the-middle only in `_40`; LCS subsequence in `_12`, longest common substring in `_25`; the Basic modular-inverse helper in `_03` and general CRT in `_38`; `minRotation` in `_10`, Lyndon factorization in `_54`; running median and removable heaps in `_52`, not in `_31`.
- Contest tools (stress tester, interactive runner, shrinking) belong to `09-Contest Testing`, not to this folder; PyRival's `tools/` is excluded here for that reason.

## Legacy migration

- `OLD/Team Notebook/src/math/old_crt.py`: `inv` is the reference for `_03.modInv`; `findMinX` (pairwise-coprime CRT) for `_38.crt`. Both legacy functions lack input validation and the non-coprime case; the active modules add them.
- `OLD/Team Notebook/src/misc/old_lcs.py`: LCS length and witness reference for `_12.lcsWitness`; the legacy prints instead of returning and uses `O(n * m)` memory.
- `OLD/Team Notebook/src/misc/old_lcssubstr.py`: longest-common-substring witness reference for `_25.longestCommonSubstring`; the active version returns spans, not a printed string.
- Legacy files stay in `OLD` until the three rows above are verified.

## Testing and omissions

- Differential and exhaustive small-case tests cover empty/negative/duplicate inputs, ties, disconnected multigraphs, overflow-sized integers, singular and composite-modulus algebra, mutation and exact/approximate distinctions; C++ variants are cross-checked without copying their assumptions. External oracles may use third-party packages; canonical modules use the standard library only.
- Deliberate omissions (add only with a concrete Python use case and an inventory update): Python Barrett/Montgomery/modint/bigint wrappers, ISA kernels, weighted blossom, dynamic planarity, segment tree beats, link-cut and Euler-tour trees, implicit treap sequence operations (range reverse), persistent union-find, high-dimensional geometry, multivariate symbolic algebra, large-FPS composition, Min_25/Meissel–Lehmer beyond Lucy, k-shortest walks, min-cost b-flow, Dirichlet series, nim product, research string indexes beyond the explicit rows.
