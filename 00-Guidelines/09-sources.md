# Research and provenance

Read this only when researching features, validating claims or adapting external material. A source list is a search plan, not evidence every source has been read.

Start from the folder's feature inventory. Compare multiple independent references, including edge conditions and less common variants; search English and Chinese/Japanese/Korean material as useful. Investigate research papers for relevant esoteric/theoretical extensions. Record what is implemented, merely planned, impractical without a separate variant, or still unverified. Do not infer correctness from popularity.

Saved author/project PDFs are indexed in [95-Resources](../95-Resources/00-index.md); download metadata records editions and checksums. A saved copy is reading material, not evidence that its algorithms have been verified.

Candidate sources:

- [cp-algorithms](https://cp-algorithms.com/), [AtCoder Library](https://github.com/atcoder/ac-library), [AtCoder tasks/editorials](https://atcoder.jp/), [Codeforces](https://codeforces.com/).
- [Yosupo Library Checker](https://judge.yosupo.jp/), [CSES](https://cses.fi/problemset/), [Luogu](https://www.luogu.com.cn/), [OI Wiki](https://oi-wiki.org/).
- [KACTL](https://github.com/kth-competitive-programming/kactl), [Nyaan](https://github.com/NyaanNyaan/library), [ei1333](https://github.com/ei1333/library), [suisen](https://github.com/suisen-cp/cp-library-cpp), [maspypy](https://github.com/maspypy/library), [hitonanode](https://github.com/hitonanode/cplib-cpp).
- [verngutz/cp-library](https://github.com/verngutz/cp-library), particularly its [contest templates](https://github.com/verngutz/cp-library/tree/master/template).
- ICPC team notebooks and university contest materials; relevant published papers/preprints; Chinese/JP/KR competitive programming articles/editorials.
- Original algorithms.cpp, [1] algorithms.cpp and older materials retained in OLD are behavioral/feature references, not authoritative specifications.

Do not use local ProgVar.pdf or frankenstein.pdf as active references. The original discussion named Ateneo/ProgVar notebooks generally, but the final decision explicitly removed these two PDFs; do not silently re-add them.

For each adopted result record a precise URL/title, the relevant algorithm/claim, publication/revision where available, and whether code was adapted or independently implemented. Preserve required attribution/license notices; keep detailed provenance in companion docs and only necessary notices in compressed headers. Benchmarks and online acceptance records are separate evidence types. Do not fabricate a citation, live inspection or acceptance.

Compiler baseline rationale: [GCC C++20 status](https://gcc.gnu.org/projects/cxx-status.html#cxx20), inspected 2026-09-27, still describes almost-full support; the chosen library floor is GCC14+, GNU++20, tested features only. Agent routing uses a short root AGENTS.md and focused docs following [official instruction-file guidance](https://developers.openai.com/codex/guides/agents-md/), inspected 2026-09-27; ordinary algorithm work does not need to load this research guide.

## Inventory audit source expansion (2026-09-27)

The [audit report](16-inventory-audit.md) explains coverage. [Retrieval/read-scope records](17-research-sources.json) distinguish catalogs, selected APIs/source comments and paper abstracts. Open the relevant entries only. The following extend the candidate list; these are source choices, not library dependencies:

- Arithmetic/algebra: [GMP algorithm manual](https://gmplib.org/manual/Algorithms), [FLINT](https://flintlib.org/doc/), [NTL](https://libntl.org/doc/tour-modules.html), [Sage reference](https://doc.sagemath.org/html/en/reference/index.html), and the saved *Modern Computer Arithmetic*. Compare domain, certification and coefficient-growth contracts as well as operation names.
- Geometry: [CGAL package catalog](https://doc.cgal.org/latest/Manual/packages.html), [Shewchuk robust predicates](https://www.cs.cmu.edu/~quake/robust.html), Stanford/KACTL notebooks and Japanese library APIs. The CGAL catalog informs reusable contest algorithms; the library is not a wholesale CAD/mesh-processing port.
- Graphs/data structures: [tko919](https://tko919.github.io/library/), [Koosaga maintained library](https://github.com/koosaga/olympiad/tree/master/Library/codes), [DYNAMIC](https://github.com/xxsds/DYNAMIC), [OGDF decompositions](https://ogdf.github.io/doc/ogdf/group__decomp.html), [NetworkX algorithm reference](https://networkx.org/documentation/stable/reference/algorithms/index.html), and the saved *Open Data Structures*. Check specialized implementations' limitations: a related algorithm name does not establish full generality.
- Strings: [Lecroq/Charras exact matching catalog](https://www-igm.univ-eiffel.fr/~lecroq/string/), [Runs Theorem](https://arxiv.org/abs/1406.0263), [r-index](https://arxiv.org/abs/1705.10382), [Optimal Dynamic Strings](https://arxiv.org/abs/1511.02612), [semi-local string comparison](https://arxiv.org/abs/0707.3619). This pass read catalog/abstract/API scope; full proof review remains implementation work.
- Python/performance: [PyRival](https://github.com/cheran-senthil/PyRival), [third-party ACL Python adaptation](https://github.com/not522/ac-library-python), [Python library reference](https://docs.python.org/3/library/), [PyPy documentation](https://doc.pypy.org/), [Algorithmica](https://en.algorithmica.org/). Check exact arithmetic and supported interpreter versions independently of reference code.

All Library Checker problem-family names at the recorded snapshot are mapped in [20-library-checker-coverage.json](20-library-checker-coverage.json). That file records inventory ownership, not accepted solutions. [19-archive-map.json](19-archive-map.json) similarly accounts for legacy material without granting it correctness.
