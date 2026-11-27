# Research and Provenance

Read for completeness research, provenance questions or adapting external code. A source list is a search plan, not a record of reading (`01-principles.md`, Evidence).

## Completeness Sweep

For a family, compare the inventory row against each catalog below, add every public operation found there that fits the family and every algorithm `01-principles.md` (Priority) admits for its profile, and record each source consulted in the folder's `Docs/00-sources.md` with a precise URL and the date.

| Area | Catalogs (Source Keys) |
|---|---|
| All | LC, CPA, ACL, KACTL, OI, CSES, BOJ, LOJ, ITMO, EMAXX, WIKI (List of algorithms, List of data structures), DADS, TAOCP, CLRS, EOA, ALGOWIKI |
| Judges and curricula | AOJ, ATCODER, CODECHEF, CP4, LEETCODE, NOIPH, PE, ROSETTA, USACO, VNOI, QOJ, SPOJ, YUKI, CMU750 |
| Japanese libraries | NYA, MAS, EI, SUI, HIT, TKO, NOSHI, BEET, NACHIA, HOS, KOPRICKY, DRKEN, ANQOOQIE, TOOMER, RSK |
| Other libraries | KOO, ECN, VERN, PYRIVAL, SHOHAG, BENQ, UFMG, TRUNG, SPAGHETTI, OLDYAN, ENDLESS, JHUI, CPAUX, DACIN, INDY, ITMOTB, TITAN, THEALGO |
| Arithmetic and algebra | GMP, FLINT, NTL, MCA, PARI, PRIMECOUNT, VZG, ARNDT, HAC, GAP, CRANDALL |
| Geometry | CGAL, SHEWCHUK, BOOST (Geometry), GEOS, DEBERG, OROURKE, HDCG |
| Graphs and structures | NX, OGDF, DYNAMIC, LEMON, IGRAPH, SAGE, SDSL, MIT6851, CS166, NAVARRO, MEHTASAHNI, AMO, SKIENA, SCHRIJVER |
| Strings | LECROQ, RUNS, RINDEX, SDSL, GUSFIELD, JEWELS |
| Performance | ALG, PYPY, HACKERS, ARNDT, saved *Optimizing C++*, *Microarchitecture*, *Instruction tables* |

A source is a catalog when it names operations itself (a tag or template-problem list, a library index, a table of contents) and a sample of it held a contest operation that neither the library nor another catalog had; a book whose table of contents alone is free is swept by it, and its body is asked of the user where an operation needs it. The [2026-11-27 survey](<23-Sweep Ledgers/Docs/00-catalog-survey.md>) records every source weighed and its verdict (user decision 2026-11-27); the candidates its samples found were applied the same day, and those of the sources the user then placed on 2026-11-28 ([omissions.md](<23-Sweep Ledgers/omissions.md>), Recorded Decisions; user decision 2026-11-28).

Saved PDFs are indexed in [95-Resources](../95-Resources/00-index.md); a source that cannot be opened is asked of the user (`01-principles.md`, Evidence). `OLD` and `97-Legacy` are behavioral references, never specifications. `ProgVar.pdf` and `frankenstein.pdf` are excluded by decision.

## Source Keys

A row without an evidence link names the sources of its operations in its Status cell by these keys, each optionally followed by a parenthesized detail (`KACTL (SolveLinearBinary)`); `03-consistency.py` rejects any other key. A key records where the operations were compared, never correctness evidence or adopted code. A folder's `Docs/00-sources.md` records what it read under each key, and a new source gets its key here first.

| Key | Source |
|---|---|
| ACL | [AtCoder Library](https://github.com/atcoder/ac-library) |
| ACLPY | [ac-library-python](https://github.com/not522/ac-library-python) |
| ALG | [Algorithmica](https://en.algorithmica.org/) |
| ALGOWIKI | [MIT Algorithm Wiki](https://algorithm-wiki.org/families), problem families |
| AMO | Ahuja, Magnanti, Orlin, *Network Flows: Theory, Algorithms, and Applications* (1993) |
| ANQOOQIE | [anqooqie proconlib](https://github.com/anqooqie/proconlib) |
| AOJ | [Aizu Online Judge](https://onlinejudge.u-aizu.ac.jp/courses/list) courses (ALDS1, DSL, GRL, CGL, NTL, DPL) |
| ARNDT | Arndt, [*Matters Computational*](https://www.jjj.de/fxt/) (fxtbook) |
| AST94 | Aggarwal, Schieber, Tokuyama, minimum-weight k-link paths with the concave Monge property, DCG (1994) |
| ATCODER | [AtCoder](https://atcoder.jp/) problems and editorials (EDPC, TDPC, Typical 90) |
| AUDIT | Candidate list supplied with the Graphs rewrite task of 2026-10-06 (`04-Graphs/Docs/00-sources.md`) |
| BEET | [beet-aizu library](https://beet-aizu.github.io/library/) |
| BENDER02 | Bender, Cole, Demaine, Farach-Colton, Zito, order maintenance, ESA 2002 |
| BENQ | [Benq cp-notebook](https://github.com/bqi343/cp-notebook) |
| BFC04 | Bender, Farach-Colton, The Level Ancestor Problem simplified, TCS (2004) |
| BKPR21 | Belazzougui, Kosolobov, Puglisi, Raman, Weighted Ancestors in Suffix Trees Revisited, CPM 2021 |
| BOJ | [Baekjoon Online Judge tags](https://www.acmicpc.net/problem/tags) (the solved.ac taxonomy) |
| BOOST | [Boost C++ Libraries documentation](https://www.boost.org/doc/libs/) (Graph, Math, Geometry) |
| BPR17 | Barbay, Pérez-Lantero, Rojas-Ledesma, depth distribution in high dimensions, arXiv 1705.10022 |
| CF | [Codeforces](https://codeforces.com/) problems and editorials |
| CGAL | [CGAL packages](https://doc.cgal.org/latest/Manual/packages.html) |
| CL86 | Chazelle, Lee, On a circle placement problem, Computing 36 (1986) |
| CLRS | Cormen, Leiserson, Rivest, Stein, *Introduction to Algorithms*, 4th edition (2022) |
| CMU750 | [CMU 15-750 Graduate Algorithms](https://www.cs.cmu.edu/~15750/), lectures and readings |
| CNBLOGS | Posts on [cnblogs.com](https://www.cnblogs.com/) |
| CODECHEF | [CodeChef](https://www.codechef.com/) problem tags |
| CP10 | Chan, Pătraşcu, counting inversions, offline orthogonal range counting, and related problems, SODA 2010 |
| CP4 | Halim, *Competitive Programming 4*, [Methods to Solve](https://cpbook.net/methodstosolve) |
| CPA | [cp-algorithms](https://cp-algorithms.com/) |
| CPAUX | [cp-algorithms-aux](https://github.com/cp-algorithms/cp-algorithms-aux) (lib.cp-algorithms.com) |
| CRANDALL | Crandall, Pomerance, *Prime Numbers: A Computational Perspective*, 2nd edition (2005) |
| CS166 | [Stanford CS166 Data Structures](https://web.stanford.edu/class/cs166/) |
| CSES | [CSES](https://cses.fi/problemset/) |
| DACIN | [dacin21 codebook](https://github.com/dacin21/dacin21_codebook) |
| DADS | NIST [Dictionary of Algorithms and Data Structures](https://xlinux.nist.gov/dads/) |
| DEBERG | de Berg, Cheong, van Kreveld, Overmars, *Computational Geometry: Algorithms and Applications*, 3rd edition (2008) |
| DMOJ | [DMOJ](https://dmoj.ca/) problems |
| DRKEN | [drken1215 algorithm](https://github.com/drken1215/algorithm) and [blog](https://drken1215.hatenablog.com/) |
| DYNAMIC | [DYNAMIC](https://github.com/xxsds/DYNAMIC) |
| DYNAMICSTRINGS | Gawrychowski et al., Optimal Dynamic Strings, arXiv 1511.02612 |
| ECN | [ecnerwala](https://github.com/ecnerwala/cp-book) |
| EI | [ei1333](https://ei1333.github.io/library/) |
| EMAXX | [e-maxx](http://e-maxx.ru/algo/), the Russian original of cp-algorithms |
| EMTHRM | [emthrm cp-library](https://emthrm.github.io/cp-library/) |
| ENDLESS | [EndlessCheng copypasta](https://github.com/EndlessCheng/codeforces-go/tree/master/copypasta) |
| EOA | Kao (ed.), [*Encyclopedia of Algorithms*](https://link.springer.com/referencework/10.1007/978-1-4939-2864-4), 2nd edition (2016) |
| ERICKSON | Erickson, [*Algorithms*](https://jeffe.cs.illinois.edu/teaching/algorithms/) and lecture notes (saved PDF) |
| FLINT | [FLINT](https://flintlib.org/doc/) |
| GAP | [GAP reference manual](https://docs.gap-system.org/doc/ref/chap0.html) (permutation groups) |
| GEOS | [GEOS C API](https://libgeos.org/doxygen/geos__c_8h.html) |
| GLN14 | Gawrychowski, Lewenstein, Nicholson, Weighted ancestors in suffix trees, ESA 2014, arXiv 1406.7716 |
| GMP | [GMP algorithms](https://gmplib.org/manual/Algorithms) |
| GR13 | Ghasemalizadeh, Razzazi, output-sensitive covering of many points, arXiv 1312.0389 |
| GUSFIELD | Gusfield, *Algorithms on Strings, Trees, and Sequences* (1997) |
| GUTING84 | Güting, contour sweep with a contracted segment tree, J. Algorithms (1984) |
| HAC | Menezes, van Oorschot, Vanstone, [*Handbook of Applied Cryptography*](https://cacr.uwaterloo.ca/hac/) |
| HACKERS | Warren, *Hacker's Delight*, 2nd edition (2012) |
| HDCG | Goodman, O'Rourke, Tóth, [*Handbook of Discrete and Computational Geometry*](https://www.csun.edu/~ctoth/Handbook/HDCG3.html), 3rd edition |
| HDT | [dynamic-connectivity-hdt](https://github.com/tomtseng/dynamic-connectivity-hdt) (Holm, de Lichtenberg, Thorup 2001) |
| HIGHNAM85 | Highnam, symmetries of planar point sets, CMU report (1985) |
| HIT | [hitonanode](https://hitonanode.github.io/cplib-cpp/) |
| HOS | [hos-lyric libra](https://github.com/hos-lyric/libra) |
| HPM05 | Har-Peled, Mazumdar, smallest k-enclosing circle, Algorithmica 41 (2005) |
| IGRAPH | [igraph C reference](https://igraph.org/c/doc/) |
| INDY | [indy256 codelibrary](https://github.com/indy256/codelibrary) |
| ITMO | [ITMO conspect wiki](https://neerc.ifmo.ru/wiki/) |
| ITMOTB | [ITMO team notebook](https://github.com/GMKirkon/ITMO_Shuffle_Duffle_Muzzle_Muff_teambook) |
| JEWELS | Crochemore, Rytter, *Jewels of Stringology* (2002) |
| JHUI | justiceHui [icpc-teamnote](https://github.com/justiceHui/icpc-teamnote) and [AlgorithmImplement](https://github.com/justiceHui/AlgorithmImplement) |
| KACTL | [KACTL](https://github.com/kth-competitive-programming/kactl) |
| KIMROTE16 | Kim, Rote, arXiv 1603.07269 |
| KOO | [Koosaga](https://github.com/koosaga/olympiad/tree/master/Library/codes) |
| KOPRICKY | [kopricky My-Algorithm](https://github.com/kopricky/My-Algorithm) |
| LC | Library Checker: [problem list](https://judge.yosupo.jp/) and [problem repository](https://github.com/yosupo06/library-checker-problems) |
| LECROQ | [Lecroq exact matching](https://www-igm.univ-eiffel.fr/~lecroq/string/) |
| LEETCODE | [LeetCode](https://leetcode.com/problemset/) topic tags |
| LEMON | [LEMON graph library](https://lemon.cs.elte.hu/pub/doc/latest/modules.html) |
| LOJ | [LibreOJ](https://loj.ac/) template problems (tag 模板) |
| LUOGU | [Luogu](https://www.luogu.com.cn/) problems (template problems 【模板】) and articles |
| MAS | [maspypy](https://maspypy.github.io/library/) |
| MCA | Brent, Zimmermann, *Modern Computer Arithmetic* (saved PDF) |
| MEGIDDO83 | Megiddo, linear-time linear programming in R^3 and related problems, SIAM J. Comput. 12 (1983) |
| MEHTASAHNI | Mehta, Sahni, *Handbook of Data Structures and Applications*, 2nd edition (2018) |
| MIT6851 | Demaine, [MIT 6.851 Advanced Data Structures](https://courses.csail.mit.edu/6.851/spring21/) |
| NACHIA | [Nachia cp-library](https://github.com/NachiaVivias/cp-library) |
| NAVARRO | Navarro, *Compact Data Structures* (2016) |
| NOIPH | NOI.PH lecture notes (saved PDFs, `95-Resources/11-NOI.PH Notes`) |
| NOSHI | [noshi91](https://noshi91.github.io/Library/) |
| NR | [Numerical Recipes](https://numerical.recipes/book.html), 3rd edition |
| NTL | [NTL](https://libntl.org/doc/tour-modules.html) |
| NX | [NetworkX reference](https://networkx.org/documentation/stable/reference/algorithms/index.html) |
| NYA | [Nyaan](https://nyaannyaan.github.io/library/) |
| OAMB86 | O'Rourke, Aggarwal, Maddila, Baldwin, minimal enclosing triangles, J. Algorithms 7 (1986) |
| ODS | Morin, *Open Data Structures* (saved PDF) |
| OGDF | [OGDF](https://ogdf.github.io/doc/ogdf/) |
| OI | [OI Wiki](https://oi-wiki.org/) |
| OLDYAN | [old-yan CP-template](https://github.com/old-yan/CP-template) |
| OROURKE | O'Rourke, *Computational Geometry in C*, 2nd edition (1998) |
| PARI | [PARI/GP reference](https://pari.math.u-bordeaux.fr/dochtml/html/toc.html) |
| PE | [Project Euler](https://projecteuler.net/archives) problems (content CC BY-NC-SA 4.0), by topic through [project-euler-categorized](https://github.com/b0o/project-euler-categorized) |
| PRIMECOUNT | [primecount](https://github.com/kimwalisch/primecount) and [primesieve](https://github.com/kimwalisch/primesieve) |
| PYPY | [PyPy docs](https://doc.pypy.org/) |
| PYRIVAL | [PyRival](https://github.com/cheran-senthil/PyRival) |
| QOJ | [QOJ](https://qoj.ac/problems?tag=%E6%A8%A1%E6%9D%BF%E9%A2%98) template problems |
| RINDEX | [r-index](https://arxiv.org/abs/1705.10382) (Gagie, Navarro, Prezza) |
| ROSETTA | [Rosetta Code programming tasks](https://rosettacode.org/wiki/Category:Programming_Tasks) |
| ROTE19 | Rote, largest contained quadrilateral and smallest enclosing parallelogram, arXiv 1905.11203 |
| RSK | [rsk0315 library-rs](https://github.com/rsk0315/library-rs) |
| RUNS | [Runs Theorem](https://arxiv.org/abs/1406.0263) |
| SAGE | [SageMath graph reference](https://doc.sagemath.org/html/en/reference/graphs/index.html) |
| SCHIEBER98 | Schieber, k-link paths with the concave Monge property, J. Algorithms 29 (1998) |
| SCHRIJVER | Schrijver, *Combinatorial Optimization: Polyhedra and Efficiency* (2003) |
| SDSL | [SDSL-lite](https://github.com/xxsds/sdsl-lite) (succinct data structures) |
| SEMILOCAL | Tiskin, Semi-local string comparison, arXiv 0707.3619 |
| SHEWCHUK | [Shewchuk predicates](https://www.cs.cmu.edu/~quake/robust.html) |
| SHOHAG | [Shahjalal Shohag code-library](https://github.com/ShahjalalShohag/code-library) |
| SKIENA | Skiena, [Stony Brook Algorithm Repository](https://www.algorist.com/algorist.html) |
| SPAGHETTI | [spaghetti-source algorithm](https://github.com/spaghetti-source/algorithm) |
| SPOJ | [SPOJ](https://www.spoj.com/problems/) |
| STANFORD | [Stanford team notebook](https://github.com/jaehyunp/stanfordacm) (saved PDF) |
| STDLIB | [Python standard library](https://docs.python.org/3/library/) |
| STVWE95 | Schwarz, Teich, Vainshtein, Welzl, Evans, minimal enclosing parallelogram, SoCG 1995 |
| SUI | [suisen](https://suisen-cp.github.io/cp-library-cpp/) |
| SYMPY | [SymPy documentation](https://docs.sympy.org/latest/) |
| TAOCP | Knuth, *The Art of Computer Programming*, volumes 4A (2011) and 4B (2022) |
| TATYAM | [tatyam SortedSet](https://github.com/tatyam-prime/SortedSet) |
| THEALGO | TheAlgorithms [C-Plus-Plus](https://github.com/TheAlgorithms/C-Plus-Plus) and [Python](https://github.com/TheAlgorithms/Python) |
| TITAN | [titan-23 Library_py](https://github.com/titan-23/Library_py) |
| TKO | [tko919](https://tko919.github.io/library/) |
| TOOMER | [to-omer competitive-library](https://github.com/to-omer/competitive-library) (Rust) |
| TOURIST | [tourist algo](https://github.com/the-tourist/algo) |
| TRUNG | [ngthanhtrung23 ACM_Notebook_new](https://github.com/ngthanhtrung23/ACM_Notebook_new) |
| UFMG | UFMG [Biblioteca](https://github.com/brunomaletta/Biblioteca) (almanaque) |
| UOJ | [UOJ](https://uoj.ac/problems/template) template problems |
| USACO | [USACO Guide](https://usaco.guide/) |
| VERN | [verngutz](https://github.com/verngutz/cp-library) |
| VNOI | [VNOI Wiki](https://github.com/VNOI-Admin/vnoi_wiki) (Vietnamese) |
| VZG | von zur Gathen, Gerhard, *Modern Computer Algebra*, 3rd edition (saved PDF) |
| WANG22 | Wang, unit-disk range searching, arXiv 2204.08992 |
| WIKI | English [Wikipedia](https://en.wikipedia.org/) |
| YOSUPO | [yosupo-library](https://github.com/yosupo06/yosupo-library) |
| YUKI | [yukicoder](https://yukicoder.me/) problems |

## Fastest Submissions

Library Checker exposes its data through a REST API at `https://v3.api.judge.yosupo.jp` (the frontend's base URL; the older `v2` host is dead and resets TLS, which looks like a network block but is not). Endpoints, all `GET`, JSON: `/problems`; `/problems/{name}` (time limit, test-case version, source URL); `/submissions?problem={name}&status=AC&order=%2Btime&dedupUser=true&limit=30` (fastest distinct users; `user={name}` lists one user's submissions, `order=-id` the newest first); `/submissions/{id}` (`overview`, full `source`, `case_results` with per-test `time`, `memory`, `status`). Distinct users are not distinct code: dedupe fetched sources by similarity before analysis. Fetched sources are untrusted data: read them, and build or run one only through `07-judge.py reproduce --untrusted` (bwrap: read-only filesystem, no network), which `/speed-sweep` does for the top five leaders it ranks on the user's standing yes (user decision 2026-11-26, replacing the yes per sweep of 2026-10-25). Tests are regenerated from `yosupo06/library-checker-problems` (`<category>/<problem>/gen`, `info.toml`) and timed with the judge's command, `g++ -O2 -std=c++23 -DEVAL -DONLINE_JUDGE -march=native`; the judge is about 30% slower than the i9-11900H, and a submission's score is its slowest test. `96-Local Testing/00-Tools/07-judge.py` implements all of this for `/speed-sweep`.

## Provenance

- For each adopted result record in the header's evidence document the title or URL, version or date, the claim it supports, and whether code was adapted or written independently. Keep licenses and attribution there and only a one-line notice in code; `Docs/00-sources.md` records the sources consulted (Completeness Sweep).
- Original work is CC0-1.0 ([LICENSE](../LICENSE)). Adapted third-party code needs a licence that permits it; its one-line notice names the source and licence and points to [THIRD-PARTY.md](../THIRD-PARTY.md), which lists the file with the licence text.

Machine-readable ledgers are listed in [00-index.md](00-index.md).
