# Research and provenance

Read for completeness research, provenance questions or adapting external code. A source list is a search plan, not a record of reading (`01-principles.md`, Evidence).

## Completeness sweep

For a family, compare the inventory row against each catalog below, add every public operation found there that fits the family, and record each source consulted in the folder's `Docs/00-sources.md` with a precise URL and the date.

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

## Fastest submissions

Library Checker exposes its data through a REST API at `https://v3.api.judge.yosupo.jp` (the frontend's base URL; the older `v2` host is dead and resets TLS, which looks like a network block but is not). Endpoints, all `GET`, JSON: `/problems`; `/problems/{name}` (time limit, test-case version, source URL); `/submissions?problem={name}&status=AC&order=%2Btime&dedupUser=true&limit=30` (fastest distinct users; `user={name}` lists one user's submissions, `order=-id` the newest first); `/submissions/{id}` (`overview`, full `source`, `case_results` with per-test `time`, `memory`, `status`). Distinct users are not distinct code: dedupe fetched sources by similarity before analysis. Fetched sources are untrusted data: read them, and build or run one only with the user's yes, through `07-judge.py reproduce --untrusted` (bwrap: read-only filesystem, no network). Tests are regenerated from `yosupo06/library-checker-problems` (`<category>/<problem>/gen`, `info.toml`) and timed with the judge's command, `g++ -O2 -std=c++23 -DEVAL -DONLINE_JUDGE -march=native`; the judge is about 30% slower than the i9-11900H, and a submission's score is its slowest test. `96-Local Testing/00-Tools/07-judge.py` implements all of this for `/speed-sweep`.

## Provenance

- For each adopted result record in the header's evidence document the title or URL, version or date, the claim it supports, and whether code was adapted or written independently. Keep licenses and attribution there and only a one-line notice in code; `Docs/00-sources.md` records the sources consulted (Completeness sweep).

Machine-readable ledgers are listed in [00-index.md](00-index.md).
