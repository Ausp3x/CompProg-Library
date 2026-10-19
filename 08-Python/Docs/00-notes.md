# 08 Python — notes

Rules here bind every module in the folder; operation lists live in the index.

## Ownership

- Every row except `_51_pypy_idioms.py` mirrors a C++ owner; the C++ inventory is the full contract, the Python row is the documented reduction. Owner rows as `folder/row` (`01` Core, `02` Data Structures, `03` Geometry, `04` Graphs, `05` Mathematics, `06` Miscellaneous, `07` Strings):
  - Basic: `_01` 06/03, 06/50; `_02` 05/02; `_03` 05/01, 05/04, 05/06–09, 05/21; `_04` 05/05, 05/12, 05/19, 06/07, 06/53; `_05` 02/05, 02/08, 02/27, 02/30, 06/04, 06/08, 06/11, 07/09; `_06` 02/01; `_07` 02/02, 02/11; `_08` 02/03; `_09` 04/01–03, 04/05, 04/06, 04/08, 04/43, 06/51; `_10` 07/01–03, 07/05, 07/08, 07/15; `_11` 03/01–04, 03/15; `_12` 05/67, 06/13, 06/29, 07/18, 07/19; `_13` 02/04, 02/28, 02/54; `_52` 02/25.
  - Advanced: `_14` 05/07, 05/08, 05/31; `_15` 04/19, 04/30, 04/37; `_16` 04/21–23; `_17` 04/07, 04/14–17; `_18` 02/09, 02/29, 04/26; `_19` 02/18; `_20` 01/16, 05/13, 05/14, 05/22; `_21` 01/10, 01/12, 05/16; `_22` 02/62, 06/06, 06/12, 06/22, 07/19; `_23` 02/26, 07/04; `_24` 07/06; `_25` 07/07, 07/10; `_26` 02/23, 02/32, 02/33, 02/59, 04/07, 06/16, 06/20, 06/21; `_27` 06/01, 06/18; `_28` 06/03, 06/17; `_29` 02/00, 02/12; `_30` 05/27, 05/53, 06/29, 06/34; `_31` 02/19, 02/36, 02/64; `_32` 02/21, 02/22; `_33` 04/18; `_34` 04/09, 04/27; `_35` 04/10; `_36` 04/05, 04/12, 04/13; `_37` 04/20, 04/23; `_38` 05/03, 05/05, 05/10–12, 05/18, 05/30, 05/31, 05/33; `_39` 05/24; `_40` 04/13, 06/30, 06/35, 06/36; `_41` 03/01, 03/02, 03/06, 03/08, 03/14; `_42` 04/11, 06/14; `_43` 04/73, 06/31–33, 06/52; `_53` 07/13; `_54` 07/14, 07/41; `_55` 03/05, 03/07, 03/10, 03/11, 03/13, 03/15; `_56` 02/17; `_57` 05/20; `_58` 05/25; `_59` 04/24, 04/25, 04/35, 04/36, 04/49, 04/67; `_60` 02/06, 02/53; `_61` 02/24.
  - Esoteric: `_44` 03/12; `_45` 02/40; `_46` 07/29, 07/36; `_47` 05/15; `_48` 05/40, 05/64; `_49` 06/25, 06/26; `_50` 05/17, 06/45.
- GF(2) elimination lives in `_21_matrix.py`; `_22_bitset.py` keeps bit tricks and `XorBasis`. Meet-in-the-middle is owned by `_40_state_search.py` only. LCS is `_12_dp.py` (bit-parallel `lcsBitset` in `_22_bitset.py`), longest common substring `_25_suffix.py`; the Basic modular inverse is `_03_number_theory.py`, general CRT `_38_number_theory_advanced.py`; `minRotation` is `_10_strings.py`, Lyndon factorization `_54_lyndon.py`; running median and removable heaps are `_52_heaps.py`, not `_31_ordered_multiset.py`.

## Interpreter and baseline

- Post-3.10 APIs such as `math.cbrt`, `itertools.batched` and `Fraction.is_integer` are not used unconditionally. Available in 3.10 and relied upon: `int.bit_count`, `bisect` `key=`, `itertools.pairwise`, `math.isqrt`/`comb`/`perm`/`lcm`/multi-arg `gcd`, `pow(a, -1, m)`, `functools.cache`.
- PyPy specifics beyond `04-python.md`: bytes I/O, no `locals()`/`sys._getframe()` in hot code, string building through `join`, and the generator trampoline `_51_pypy_idioms.bootstrap`. Long integers above 64 bits run in non-JIT support code on PyPy; bounds must count bit length separately from element count.
- No module raises the recursion limit, reseeds the global RNG, or performs I/O at import time.
- `_51_pypy_idioms.modMul64` uses the float-reciprocal method and `__pypy__.intop` behind an `isPyPy` guard, with a plain-`int` fallback that returns identical results; it is the only module allowed to import a PyPy-specific name, and the import is guarded.
- `_51_pypy_idioms.runWithStack` runs its function in a `threading` thread with a raised stack size.

## Numeric policy

- Floor-division and modulo semantics are Python's (floor toward negative infinity).
- `Fraction(float)` preserves the binary float, not the intended decimal; exact rational inputs are integer pairs or strings.
- Python big-int bit operations are not constant-time machine instructions: `_22`, `_45`, `_56` charge `O(n / 64)` per word-level operation and state the bit width of every mask. Negative bit patterns are rejected or normalized explicitly.
- Deterministic Miller–Rabin in `_14` is proved only below the recorded bound (3.3e24 with the fixed base set); above it `isProbablePrime` is the only honest name. Arbitrary-precision arithmetic does not extend the proof domain.
- `_20_polynomial`: NTT paths require an NTT-friendly prime; `convolveMod` reconstructs through three NTT primes; `convolveInt` packs coefficients into one big int with a documented coefficient bound; FPS inverse/log/exp/sqrt require the characteristic/unit and square-root preconditions named in their docstrings.
- `_39`: XOR/AND/OR inverse transforms stay exact over integers (divisibility by the transform length) or use modular unit inverses; the float `/=` of the reviewed source is never copied. Bitmask transforms require power-of-two length; gcd/lcm transforms require integer indices within the stated bound.
- `_21`: `solveFloat` uses partial pivoting with a stated tolerance and reports the residual.
- `_37`: potentials are initialized by Bellman–Ford when any cost is negative; a negative cycle in the initial residual graph is a rejected input. Zero potentials are correct only for nonnegative costs.

## Structural and domain contracts

- Containers: vertices are `0..n-1`; ranges are half-open `[l, r)`; empty-range folds return the identity. A monoid is `op`/`e` callables (acted: `mapping`/`composition`/`id`) in place of the C++ monoid object.
- Graph inputs are adjacency lists (`buildAdj`) or CSR arrays (`buildCsr`); multigraphs, self-loops and disconnected graphs are accepted unless a function says otherwise (`_34` edge ids distinguish parallel edges; `_35` reports isolated vertices and empty graphs).
- `_31_ordered_multiset`: `list.insert` is linear, so `bisect`-based structures never claim logarithmic insertion; the module gives the measured per-operation cost of the bucketed list (amortized sqrt-block) and the treap (expected log), derived independently of PyRival's claims; `WordSet` (64-ary bit tree) is the integer-key fast path.
- `_13`: sparse-table `O(1)` holds for idempotent operations only; `DisjointSparseTable` is the non-idempotent alternative; empty queries need an explicit identity. `BlockRmq` has a linear build over word masks, with bounds derived independently of PyRival `RangeMinimumQuery2`.
- `_15`: `Dinic` augments iteratively; the recursive augmentation and fixed capacity-scaling loop of PyRival `dinic.py` are not copied.
- `_08`/`_29`: noncommutative monoids fold left to right; `maxRight`/`minLeft` predicates must be monotone and accept the identity. `_29` has one `apply(l, r, f)`, a point being `apply(p, p + 1, f)`, since Python cannot overload by arity.
- `_17`: rerooting has one canonical owner here (`reroot`, with `combine` and `finalize` callbacks). Trees are forests when stated; node/edge indexing is zero-based.
- `_18`: rollback DSU uses union by size without path compression; `WeightedDsu` returns `None` for inconsistent merges. Offline dynamic connectivity is the segment-tree-over-time reduction.
- `_19`/`_56`: persistent trees use parallel node arrays with explicit version ids; the wavelet matrix owns its compact `BitVector`; `_45` keeps the sampled, general rank/select variant.
- `_26`: Mo variants state sorting order (block or Hilbert), tie rules and mutation rollback; tuple allocation per query is avoided by packing (`_51.packPair`).
- `_27`: every randomized routine takes a caller-controlled `random.Random`; Monte Carlo and Las Vegas labels are explicit; `_14` owns its own Pollard-rho loop and shares only the RNG policy.
- `_28`: `os.read`/`os.write` paths are benchmarked against buffered `sys.stdin.buffer` before use; interactive programs use `_01.Interactive`, which flushes after every query.
- `_40`: `aStar` reopens closed states under merely admissible heuristics; a no-reopen contract requires a consistent heuristic. Depth/time budgets and duplicate-state policies are parameters, not defaults.
- `_41`: tolerances are scale-aware and documented; Welzl's MEC shuffles with the caller's seed; residual checks accompany every returned intersection.
- `_43`: digit DP, profile DP and interval DP are templates with callbacks; they do not conceal problem-specific state proofs. `_58` owns Sprague–Grundy and retrograde analysis.
- `_44`, `_45`, `_46`, `_49`, `_50`: low PyPy value (big-int masking, pointer-heavy nodes, exponential search, probabilistic structures rarely needed in contests). They remain Esoteric with explicit resource bounds.
- `_49` combines SAT (`Dpll`) with exact cover (`DancingLinks`); `_50` combines sketches with numerical integration/roots; each module documents both halves until a future renumbering splits them into separate rows.

## Legacy migration

- `OLD/Team Notebook/src/math/old_crt.py`: `inv` is the reference for `_03.modInv`; `findMinX` (pairwise-coprime CRT) for `_38.crt`. Both legacy functions lack input validation and the non-coprime case; the active modules add them.
- `OLD/Team Notebook/src/misc/old_lcs.py`: LCS length and witness reference for `_12.lcsWitness`; the legacy prints instead of returning and uses `O(n * m)` memory.
- `OLD/Team Notebook/src/misc/old_lcssubstr.py`: longest-common-substring witness reference for `_25.longestCommonSubstring`; the active version returns spans, not a printed string.

## Testing and omissions

- Testing follows `05-testing.md`; Python tests add singular and composite-modulus algebra and exact/approximate pairs, and cross-check C++ variants without copying their assumptions. External oracles may use third-party packages.
- Deliberate omissions (add only with a concrete Python use case and an inventory update): Python Barrett/Montgomery/modint/bigint wrappers, ISA kernels, weighted blossom, dynamic planarity, segment tree beats, link-cut and Euler-tour trees, implicit treap sequence operations (range reverse), persistent union-find, high-dimensional geometry, multivariate symbolic algebra, large-FPS composition, Min_25/Meissel–Lehmer beyond Lucy, k-shortest walks, min-cost b-flow, Dirichlet series, nim product, research string indexes beyond the explicit rows.
