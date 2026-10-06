# Research and provenance

Read for completeness research, provenance questions or adapting external code. A source list is a search plan; cite only what was actually read.

## Completeness sweep

For a family, compare the inventory row against each catalog below, add every public operation found there that fits the family, and record each source consulted in the folder's `81-sources.md` with a precise URL and the date.

| Area | Catalogs |
|---|---|
| All | [Library Checker problem list](https://judge.yosupo.jp/), [cp-algorithms](https://cp-algorithms.com/), [AtCoder Library](https://github.com/atcoder/ac-library), [KACTL](https://github.com/kth-competitive-programming/kactl), [OI Wiki](https://oi-wiki.org/), [CSES](https://cses.fi/problemset/) |
| Japanese libraries | [Nyaan](https://nyaannyaan.github.io/library/), [maspypy](https://maspypy.github.io/library/), [ei1333](https://ei1333.github.io/library/), [suisen](https://suisen-cp.github.io/cp-library-cpp/), [hitonanode](https://hitonanode.github.io/cplib-cpp/), [tko919](https://tko919.github.io/library/), [noshi91](https://noshi91.github.io/Library/) |
| Other libraries | [Koosaga](https://github.com/koosaga/olympiad/tree/master/Library/codes), [ecnerwala](https://github.com/ecnerwala/cp-book), [verngutz](https://github.com/verngutz/cp-library), [PyRival](https://github.com/cheran-senthil/PyRival) |
| Arithmetic and algebra | [GMP algorithms](https://gmplib.org/manual/Algorithms), [FLINT](https://flintlib.org/doc/), [NTL](https://libntl.org/doc/tour-modules.html), saved *Modern Computer Arithmetic* |
| Geometry | [CGAL packages](https://doc.cgal.org/latest/Manual/packages.html), [Shewchuk predicates](https://www.cs.cmu.edu/~quake/robust.html) |
| Graphs and structures | [NetworkX reference](https://networkx.org/documentation/stable/reference/algorithms/index.html), [OGDF](https://ogdf.github.io/doc/ogdf/), [DYNAMIC](https://github.com/xxsds/DYNAMIC) |
| Strings | [Lecroq exact matching](https://www-igm.univ-eiffel.fr/~lecroq/string/), [Runs Theorem](https://arxiv.org/abs/1406.0263), [r-index](https://arxiv.org/abs/1705.10382) |
| Performance | [Algorithmica](https://en.algorithmica.org/), saved *Optimizing C++*, *Microarchitecture*, *Instruction tables*, [PyPy docs](https://doc.pypy.org/) |

Saved PDFs are indexed in [95-Resources](../95-Resources/00-index.md). `OLD` and `97-Legacy` are behavioral references, never specifications. `ProgVar.pdf` and `frankenstein.pdf` are excluded by decision.

## Provenance

- For each adopted result record title or URL, version or date, the claim it supports, and whether code was adapted or written independently. Keep licenses and attribution; put detail in `81-sources.md` and only a one-line notice in code.
- Benchmarks, tests and judge acceptance are separate evidence types; never substitute one for another.
- Never fabricate a citation, an inspection, a run or an acceptance.

Machine-readable ledgers: [17-research-sources.json](17-research-sources.json) (read-scope records), [20-library-checker-coverage.json](20-library-checker-coverage.json) (Library Checker families mapped to owners), [19-archive-map.json](19-archive-map.json) (archive accounting).
