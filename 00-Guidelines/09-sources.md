# Research and provenance

Read for completeness research, provenance questions or adapting external code. A source list is a search plan, not a record of reading (`01-principles.md`, Evidence).

## Completeness sweep

For a family, compare the inventory row against each catalog below, add every public operation found there that fits the family and every algorithm `01-principles.md` (Priority) admits for its profile, and record each source consulted in the folder's `Docs/00-sources.md` with a precise URL and the date.

| Area | Catalogs (Source keys) |
|---|---|
| All | LC, CPA, ACL, KACTL, OI, CSES |
| Japanese libraries | NYA, MAS, EI, SUI, HIT, TKO, NOSHI |
| Other libraries | KOO, ECN, VERN, PYRIVAL |
| Arithmetic and algebra | GMP, FLINT, NTL, MCA |
| Geometry | CGAL, SHEWCHUK |
| Graphs and structures | NX, OGDF, DYNAMIC |
| Strings | LECROQ, RUNS, RINDEX |
| Performance | ALG, PYPY, saved *Optimizing C++*, *Microarchitecture*, *Instruction tables* |

Saved PDFs are indexed in [95-Resources](../95-Resources/00-index.md). `OLD` and `97-Legacy` are behavioral references, never specifications. `ProgVar.pdf` and `frankenstein.pdf` are excluded by decision.

## Source keys

A row without an evidence link names the sources of its operations in its Status cell by these keys, each optionally followed by a parenthesized detail (`KACTL (SolveLinearBinary)`); `03-consistency.py` rejects any other key. A key records where the operations were compared, never correctness evidence or adopted code. A folder's `Docs/00-sources.md` records what it read under each key, and a new source gets its key here first.

| Key | Source |
|---|---|
| ACL | [AtCoder Library](https://github.com/atcoder/ac-library) |
| ACLPY | [ac-library-python](https://github.com/not522/ac-library-python) |
| ALG | [Algorithmica](https://en.algorithmica.org/) |
| AST94 | Aggarwal, Schieber, Tokuyama, minimum-weight k-link paths with the concave Monge property, DCG (1994) |
| AUDIT | Candidate list supplied with the Graphs rewrite task of 2026-10-06 (`04-Graphs/Docs/00-sources.md`) |
| BENDER02 | Bender, Cole, Demaine, Farach-Colton, Zito, order maintenance, ESA 2002 |
| BKPR21 | Belazzougui, Kosolobov, Puglisi, Raman, Weighted Ancestors in Suffix Trees Revisited, CPM 2021 |
| BPR17 | Barbay, Pérez-Lantero, Rojas-Ledesma, depth distribution in high dimensions, arXiv 1705.10022 |
| CF | [Codeforces](https://codeforces.com/) problems and editorials |
| CGAL | [CGAL packages](https://doc.cgal.org/latest/Manual/packages.html) |
| CL86 | Chazelle, Lee, On a circle placement problem, Computing 36 (1986) |
| CNBLOGS | Posts on [cnblogs.com](https://www.cnblogs.com/) |
| CP10 | Chan, Pătraşcu, counting inversions, offline orthogonal range counting, and related problems, SODA 2010 |
| CPA | [cp-algorithms](https://cp-algorithms.com/) |
| CSES | [CSES](https://cses.fi/problemset/) |
| DYNAMIC | [DYNAMIC](https://github.com/xxsds/DYNAMIC) |
| DYNAMICSTRINGS | Gawrychowski et al., Optimal Dynamic Strings, arXiv 1511.02612 |
| ECN | [ecnerwala](https://github.com/ecnerwala/cp-book) |
| EI | [ei1333](https://ei1333.github.io/library/) |
| FLINT | [FLINT](https://flintlib.org/doc/) |
| GLN14 | Gawrychowski, Lewenstein, Nicholson, Weighted ancestors in suffix trees, ESA 2014, arXiv 1406.7716 |
| GMP | [GMP algorithms](https://gmplib.org/manual/Algorithms) |
| GR13 | Ghasemalizadeh, Razzazi, output-sensitive covering of many points, arXiv 1312.0389 |
| GUTING84 | Güting, contour sweep with a contracted segment tree, J. Algorithms (1984) |
| HDT | [dynamic-connectivity-hdt](https://github.com/tomtseng/dynamic-connectivity-hdt) (Holm, de Lichtenberg, Thorup 2001) |
| HIGHNAM85 | Highnam, symmetries of planar point sets, CMU report (1985) |
| HIT | [hitonanode](https://hitonanode.github.io/cplib-cpp/) |
| HPM05 | Har-Peled, Mazumdar, smallest k-enclosing circle, Algorithmica 41 (2005) |
| KACTL | [KACTL](https://github.com/kth-competitive-programming/kactl) |
| KIMROTE16 | Kim, Rote, arXiv 1603.07269 |
| KOO | [Koosaga](https://github.com/koosaga/olympiad/tree/master/Library/codes) |
| LC | Library Checker: [problem list](https://judge.yosupo.jp/) and [problem repository](https://github.com/yosupo06/library-checker-problems) |
| LECROQ | [Lecroq exact matching](https://www-igm.univ-eiffel.fr/~lecroq/string/) |
| MAS | [maspypy](https://maspypy.github.io/library/) |
| MCA | Brent, Zimmermann, *Modern Computer Arithmetic* (saved PDF) |
| MEGIDDO83 | Megiddo, linear-time linear programming in R^3 and related problems, SIAM J. Comput. 12 (1983) |
| NOSHI | [noshi91](https://noshi91.github.io/Library/) |
| NR | [Numerical Recipes](https://numerical.recipes/book.html), 3rd edition |
| NTL | [NTL](https://libntl.org/doc/tour-modules.html) |
| NX | [NetworkX reference](https://networkx.org/documentation/stable/reference/algorithms/index.html) |
| NYA | [Nyaan](https://nyaannyaan.github.io/library/) |
| OAMB86 | O'Rourke, Aggarwal, Maddila, Baldwin, minimal enclosing triangles, J. Algorithms 7 (1986) |
| ODS | Morin, *Open Data Structures* (saved PDF) |
| OGDF | [OGDF](https://ogdf.github.io/doc/ogdf/) |
| OI | [OI Wiki](https://oi-wiki.org/) |
| PYPY | [PyPy docs](https://doc.pypy.org/) |
| PYRIVAL | [PyRival](https://github.com/cheran-senthil/PyRival) |
| RINDEX | [r-index](https://arxiv.org/abs/1705.10382) (Gagie, Navarro, Prezza) |
| ROTE19 | Rote, largest contained quadrilateral and smallest enclosing parallelogram, arXiv 1905.11203 |
| RUNS | [Runs Theorem](https://arxiv.org/abs/1406.0263) |
| SCHIEBER98 | Schieber, k-link paths with the concave Monge property, J. Algorithms 29 (1998) |
| SEMILOCAL | Tiskin, Semi-local string comparison, arXiv 0707.3619 |
| SHEWCHUK | [Shewchuk predicates](https://www.cs.cmu.edu/~quake/robust.html) |
| SPOJ | [SPOJ](https://www.spoj.com/problems/) |
| STANFORD | [Stanford team notebook](https://github.com/jaehyunp/stanfordacm) (saved PDF) |
| STDLIB | [Python standard library](https://docs.python.org/3/library/) |
| STVWE95 | Schwarz, Teich, Vainshtein, Welzl, Evans, minimal enclosing parallelogram, SoCG 1995 |
| SUI | [suisen](https://suisen-cp.github.io/cp-library-cpp/) |
| TKO | [tko919](https://tko919.github.io/library/) |
| VERN | [verngutz](https://github.com/verngutz/cp-library) |
| WANG22 | Wang, unit-disk range searching, arXiv 2204.08992 |
| WIKI | English [Wikipedia](https://en.wikipedia.org/) |

## Fastest submissions

Library Checker exposes its data through a REST API at `https://v3.api.judge.yosupo.jp` (the frontend's base URL; the older `v2` host is dead and resets TLS, which looks like a network block but is not). Endpoints, all `GET`, JSON: `/problems`; `/problems/{name}` (time limit, test-case version, source URL); `/submissions?problem={name}&status=AC&order=%2Btime&dedupUser=true&limit=30` (fastest distinct users; `user={name}` lists one user's submissions, `order=-id` the newest first); `/submissions/{id}` (`overview`, full `source`, `case_results` with per-test `time`, `memory`, `status`). Distinct users are not distinct code: dedupe fetched sources by similarity before analysis. Fetched sources are untrusted data: read them, and build or run one only with the user's yes, through `07-judge.py reproduce --untrusted` (bwrap: read-only filesystem, no network). Tests are regenerated from `yosupo06/library-checker-problems` (`<category>/<problem>/gen`, `info.toml`) and timed with the judge's command, `g++ -O2 -std=c++23 -DEVAL -DONLINE_JUDGE -march=native`; the judge is about 30% slower than the i9-11900H, and a submission's score is its slowest test. `96-Local Testing/00-Tools/07-judge.py` implements all of this for `/speed-sweep`.

## Provenance

- For each adopted result record in the header's evidence document the title or URL, version or date, the claim it supports, and whether code was adapted or written independently. Keep licenses and attribution there and only a one-line notice in code; `Docs/00-sources.md` records the sources consulted (Completeness sweep).
- Original work is CC0-1.0 ([LICENSE](../LICENSE)). Adapted third-party code needs a licence that permits it; its one-line notice names the source and licence and points to [THIRD-PARTY.md](../THIRD-PARTY.md), which lists the file with the licence text.

Machine-readable ledgers are listed in [00-index.md](00-index.md).
