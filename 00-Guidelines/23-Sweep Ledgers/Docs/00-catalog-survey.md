# Catalog Survey 2026-11-27

The user asked on 2026-11-27 for the completeness-sweep catalog list (`09-sources.md`, Completeness Sweep) to be made complete, Project Euler included, by going through judges, GitHub libraries, books and anything else missing. Five read-only agents weighed about 130 sources. For each one that names operations they sampled its most specialised 15 to 120 names and searched each with `rg -i` (synonyms and camelCase forms) over every `0[1-8]*/00-index.md`, `0[1-8]*/Docs/00-notes.md` and `omissions.md`; a name owned by any inventory under any name, or another algorithm for an operation a row already has, counted as covered. About 2,400 names were sampled and about 390 were absent, some found by several agents and some outside contest scope; the user decided the same day to apply them at once rather than at the next completeness sweep, and Applied below records the result (user decision 2026-11-27). The user then placed the sources the agents could not open, and on 2026-11-28 had them sampled and applied the same way (user decision 2026-11-28); Sampled and Applied 2026-11-28 records that run.

## Contracts

- **Catalog rule.** A source became a catalog when it names operations itself (a tag or template-problem list, a library index, a table of contents) and its sample held a contest operation that neither the library nor another catalog had. A source that failed the rule kept or got a source key when the folder ledgers already cite it or it holds verification problems, and was otherwise left out. The rule was this session's choice; its alternative, the agents' stricter yield thresholds (about ten absent operations, or two), would have left out about 30 of the 63.
- **Scope.** Samples are not exhaustive: the candidates are what the samples found, and a sweep reads each catalog in full. Absence was checked against inventories, notes and the omissions ledger, not against code, so a few candidates may exist under names the rows do not use.
- **Out of contest scope.** Cryptographic hashes and classical ciphers, check digits (Luhn, Damm, Verhoeff), Jaro–Winkler and phonetic codes, k-means, ant colony, ellipsoidal geodesics, Quine–McCluskey, Knuth–Bendix, the LFU cache and the date of Easter were found absent; they joined as Esoteric operations in algorithmic form (user decision 2026-11-27).

### Catalogs Added

Counts are sampled and absent names; each source's key is in `09-sources.md` (Source Keys). A count after a semicolon is the 2026-11-28 sample of the copy the user placed, and keys marked 2026-11-28 became catalogs then.

| Area | Source (sampled / absent) |
|---|---|
| All | BOJ 57/1; 194/2 (solved.ac, 230 tags), LOJ 38/7, ITMO about 120/16, EMAXX 145 articles/3, WIKI lists 80/45 (mostly second implementations), DADS 45/5, TAOCP 25/2; 4A 150/30, 4B 85/18, CLRS 68/9 (2026-11-28), EOA 633 entries/188 (2026-11-28), ALGOWIKI 172 families/62 (2026-11-28) |
| Judges and curricula | AOJ 25/2, ATCODER 18/1, CODECHEF 37/2, CP4 27/3, LEETCODE 13/1 (55 tags seen); 120/1 (176 tags), NOIPH 16/2 (8 of 29 notes), PE 34/3, ROSETTA 63/9 (6 in scope), USACO 33/1, VNOI 22/1, QOJ 31/1 (2026-11-28), SPOJ 62/3 (2026-11-28), YUKI 250/8 (2026-11-28), CMU750 35/3 (2026-11-28) |
| Japanese libraries | BEET 37/7, NACHIA 15/1, HOS 35/6, KOPRICKY 38/8, DRKEN 38/3, ANQOOQIE 33/4, TOOMER 36/6, RSK 34/3 |
| Other libraries | SHOHAG about 110/22, BENQ 23/2, UFMG 26/3, TRUNG 24/2, SPAGHETTI 40/17, OLDYAN 31/5, ENDLESS 35/15, JHUI 16/5, CPAUX 10/4, DACIN 30/2, INDY 36/4, ITMOTB 32/4, TITAN 33/2, THEALGO 31/16 (mostly out of scope) |
| Arithmetic and algebra | PARI 38/12, PRIMECOUNT 14/7, VZG 60/5, ARNDT 90/6, HAC 65/6, GAP chapter 43/1, CRANDALL 150/51 (2026-11-28) |
| Geometry | BOOST Geometry 15/5, GEOS 55/2, DEBERG 25/3; 62/20, OROURKE 35/2; 50/20, HDCG 68 chapters/3; 140/86 |
| Graphs and structures | LEMON 32/10, IGRAPH 42/11, SAGE 20/14, SDSL 23/15, MIT6851 25/9, CS166 25/5, NAVARRO 35/8; 188/44, MEHTASAHNI 40/5; 240/103, AMO 35/3; 61/11, SKIENA 75 problems/2, SCHRIJVER 170/54 (2026-11-28) |
| Strings | GUSFIELD 30/3; 82/24, JEWELS 20/2; 62/15 |
| Performance | HACKERS 40/1 (compress and expand); 154/28 |

### Keys without a Catalog

- Cited in the folder ledgers, no new operation in the sample: EMTHRM (29/0), YOSUPO (10/0), TOURIST (6/0), TATYAM, UOJ (6/0), DMOJ (21 broad types). ERICKSON (20/1) and LUOGU (78/1, the widest template list) found only operations other catalogs name (Karger; online tree blocking). CF stays a key: Codeforces tags are coarse and its catalog blog adds nothing.

### Left Out

- No specialised names: HackerRank (14 coarse subdomains), Kattis (no topic tags), TLX (basic chapter names), DMOJ types, *Concrete Mathematics*, Flajolet–Sedgewick (asymptotic theory), CMU 15-451 (the saved page lists only course offerings, 2026-11-28).
- Fully covered, or covered by another catalog: hly1204, zimpha (CC BY-NC-ND), the Competitive Programmer's Handbook (the CSES companion) and its successor, Laaksonen's *Guide to Competitive Programming*, 3rd edition (95/0, 2026-11-28), MIT 6.854 (FKS hashing also in MIT6851), graph-tool (percolation also in IGRAPH), OR-Tools, Clipper2, SNAP, Cohen's *Course in Computational Algebraic Number Theory*.
- Discovered and ranked below the evaluated libraries (small, inactive or overlapping): kemuniku cplib (Nim), sotanishy, noya2, hotman78, ningenMe, ruthen71, potato167, mugen1337, KowerKoint, kmyk, ac-adapter-rs, EbTech rust-algorithms, ac-library-rs, kenkoooo and tanakh Rust libraries, Neterukun1993 (Python), bcw_codebook, SuprDewd, el-vasito, programming_team_code, radoslav11, maksim1744, ekzhang, bicsi, std_abs, gratus907, nealwu, AhmadElsagheer, mochow13, xcpc-algorithm-templates, EgorKulikov yaal, antonkov ITMOTeamBook.

## Applied 2026-11-27

Every candidate the samples found was applied on 2026-11-27 under `/omission-sweep` Apply (user decision 2026-11-27). One agent per folder checked each candidate against the inventories, the folder notes, `omissions.md` and, where a decision needed it, the catalog's own file. A second agent per folder checked every decision and corrected 44 of them (an established name, the owning row or folder, or a record the first agent missed), and each correction was taken. Items found in two folders were merged: the BK-tree, the octree, the persistent centroid decomposition and the M-tree. The user decided the two questions the rules left open: structures whose only gain is space join as Esoteric operations, and the planar Gomory–Hu tree joins row 46 (`omissions.md`, Recorded Decisions, 2026-11-27).

| Folder | Items Added to Rows | Rows Extended | Items in New Rows | Left Out | Already Present | Speed Notes |
|---|---|---|---|---|---|---|
| 01 Core | 0 | 0 | 0 | 0 | 0 | 1 |
| 02 Data Structures | 30 | 18 | 6 | 13 | 2 | 1 |
| 03 Geometry | 32 | 13 | 8 | 1 | 6 | 1 |
| 04 Graphs | 92 | 38 | 0 | 2 | 6 | 1 |
| 05 Mathematics | 84 | 36 | 10 | 2 | 4 | 2 |
| 06 Miscellaneous | 68 | 23 | 14 | 0 | 3 | 0 |
| 07 Strings | 22 | 13 | 8 | 3 | 0 | 0 |
| 09 Contest Testing | 0 | 0 | 0 | 0 | 1 | 0 |

The 02 row count includes `RunningMedian` in row 25, the C++ form of the `08-Python` `_052_heaps.py` structure that Findings named. An item is one candidate operation or type; each row's added operations are in its Operations cell and its package's note or follow-up. Left-out items are in each folder's `Docs/00-notes.md` (Omission Sweep 2026-11-27), and the fetched pages in each `Docs/00-sources.md` (Fetched 2026-11-27).

- **Reopened Rows.** Sixteen verified rows became `partial` with the new operations after `missing:`; their packages stay verified with a follow-up: P006, P010, P012, P013, P015 and P016. One verified Geometry row belongs to an in-progress package, whose note carries the work.
- **New Rows.** Fifteen rows, each with a new batch and package: Data Structures `076-classic_structures.hpp` (P291) and `077-problem_structures.hpp` (P292); Geometry `078-ellipsoidal_geodesics.hpp` (P293), `079-geometric_coresets.hpp` (P294), `080-geometry_problems.hpp` (P295) and `081-polyomino_enumeration.hpp` (P296); Mathematics `076-addition_chains.hpp` (P297), `077-combinatorial_designs.hpp` (P298), `078-symbolic_summation.hpp` (P299) and `079-thueequation.hpp` (P300); Miscellaneous `061-connected_component_dp.hpp` (P301, Advanced), `062-cryptography.hpp` (P302) and `063-decision_diagrams.hpp` (P303); Strings `057-anti_hash.hpp` (P304) and `058-string_similarity.hpp` (P305). All but P301 are Esoteric.
- **Speed Notes.** Faster forms of existing operations, each for its owner to measure and decide (`05-testing.md`, Benchmarks): thin heaps against `FibonacciHeap` (P029), the MVP tree against `VpTree` (P040), leaf peeling for `rootTree` (P042), the bucket sieve for `SegmentedSieve` (P012, follow-up), block inversion for Core `Matrix::inv` (P020) and the Chakravala method for `pellFundamental` (P178). The near-linear modular subset sum, Gourdon prime counting and Dinic with dynamic trees became operations of their rows, each kept only on a measured crossover.

## Sampled and Applied 2026-11-28

The user placed 24 of the sources the agents could not open, in 31 files (solved.ac as eight saved pages), and ordered them sampled now and applied under `/omission-sweep` Apply like the candidates of 2026-11-27 (user decision 2026-11-28). Two read-only finders read the judge lists, the curricula and the general books (CLRS, the *Guide*, the *Encyclopedia*); one agent per folder read the full contents of its topic books, sampled their specialised names, confirmed each candidate absent against the rows, notes, `omissions.md` and the code, and decided it; a second agent per folder checked every decision and skimmed the books again. The second agents corrected 75 decisions and found 41 more items, and every correction was taken. 23 items found by two folders were kept once, in the row their owning folder chose (among them the Even–Shiloach tree, the drawing operations, the dynamic FM-index's document operations, textbook RSA and the CTL model checker), and the machine-learning rows proposed for Mathematics and Miscellaneous became one Miscellaneous row.

| Folder | Items Added to Rows | Rows Extended | Items in New Rows | Left Out | Already Present | Speed Notes |
|---|---|---|---|---|---|---|
| 01 Core | 0 | 0 | 0 | 1 | 4 | 3 |
| 02 Data Structures | 24 | 9 | 8 | 11 | 7 | 4 |
| 03 Geometry | 101 | 36 | 49 | 7 | 26 | 3 |
| 04 Graphs | 142 | 48 | 61 | 4 | 9 | 1 |
| 05 Mathematics | 62 | 25 | 18 | 3 | 4 | 9 |
| 06 Miscellaneous | 127 | 31 | 49 | 4 | 5 | 3 |
| 07 Strings | 68 | 20 | 21 | 3 | 12 | 0 |

An item is one candidate operation or type; each row's added operations are in its Operations cell and its package's note or follow-up. Left-out items are in each folder's `Docs/00-notes.md` (Omission Sweep 2026-11-28), and the sources each folder's items came from in its `Docs/00-sources.md` (Fetched 2026-11-28).

- **Reopened Rows.** Seven verified rows became `partial` with the new operations after `missing:` (Geometry rows 02 and 03, Graphs rows 09, 10, 11 and 29, Miscellaneous row 60); their packages stay verified with a follow-up: P002, P010, P011, P013 and P015, and P018 for the InfInt speed note.
- **New Rows.** 27 rows, each with a new batch and package, all Esoteric: Data Structures `078-set_families.hpp` (P306) and `079-succinct_graphs.hpp` (P307); Geometry `082-collision_detection.hpp` to `093-polygon_placement.hpp` (P308–P319); Mathematics `080-multiplicative_weights.hpp`, `081-sparse_elimination.hpp` and `082-strategic_games.hpp` (P320–P322); Miscellaneous `064-approximation_algorithms.hpp` to `071-operating_systems.hpp` (P323–P330); Strings `059-bioinformatics.hpp` and `060-rna_secondary_structure.hpp` (P331, P332).
- **Graphs Rows.** The ten Graphs rows the run proposed (61 items: dynamic matching, dynamic shortest paths, graph drawing, graph partitioning, maximum cut, metric embeddings, phylogenetics, planar algorithms, shortest-path preprocessing and spanning-tree problems) needed prefixes `04-Graphs` no longer had, so they were planned with the three-digit prefixes the user adopted the same day (`10-decisions.md`, 2026-11-28 Three-Digit Prefixes).
- **Speed Notes.** Faster forms of existing operations, each for its owner to measure and decide (`05-testing.md`, Benchmarks), among them three Core full candidates: the Stehlé–Zimmermann gcd for `InfInt` (P018, follow-up), the Harley–Seal population count for `Bitset` and the block-swap transpose for `BitMatrix`.
- **Settled without the User.** Vertex-connectivity augmentation (SCHRIJVER) joined Graphs row 45, since the P047 omission's only reason, that no catalog had it, no longer held; the agents had left it to the user.

## Findings

- **Records.** Tonelli–Shanks is decided in `05-Mathematics/Docs/011-discrete_log_root.md` (row only, 2026-10-16) rather than in the notes. The Mahonian-numbers hand-off to P054 (`omissions.md`, 2026-11-17) waits on P054, which is in progress. The `pdep`/`pext` and auction findings, the C++ `RunningMedian` and LOJ 174 (`egfConvolution` asserts a prime modulus, so row 12 gained `egfConvolutionCompositeMod`) were settled in Applied.

## Sources

The agents fetched each catalog's index, tag list or table of contents at the URL its key gives; Luogu, LibreOJ, Codeforces, LeetCode, TLX, DMOJ, CodeChef, Rosetta Code and Aizu through their JSON APIs. Saved files read: `95-Resources/01-competitive_programmers_handbook-laaksonen.pdf` (contents, pages 1–8), eight of the NOI.PH notes (section headings), `13-Papers/001-modern_computer_algebra-von_zur_gathen_2013.pdf` (contents, pages 1–15), `13-Papers/002-computational_algebraic_number_theory-cohen_1993.pdf` (contents, pages 1–22) and `02-algorithms-erickson.pdf`. The documents and catalog index pages the agents downloaded were filed on 2026-11-27 (user decision 2026-11-27; fetched by Claude, not user-placed); each folder of pages is one zip of the files as fetched. Repository tree listings, API dumps used only for counting and text extracts were not kept. The sources the user placed were filed on 2026-11-28 (user-placed), each read as its row says.

| Title | URL | File and SHA-256 | Read |
|---|---|---|---|
| Menezes, van Oorschot, Vanstone, *Handbook of Applied Cryptography* (1996), chapter 3, Number-Theoretic Reference Problems | https://cacr.uwaterloo.ca/hac/about/chap3.pdf | `95-Resources/13-Papers/052-handbook_of_applied_cryptography_ch3-menezes_1996.pdf`, `1ea867655cdac2285ac7d2fe0aaf8b1f726fa181777553344de83b72fefb210c` | algorithm titles |
| The same, chapter 4, Public-Key Parameters | https://cacr.uwaterloo.ca/hac/about/chap4.pdf | `95-Resources/13-Papers/053-handbook_of_applied_cryptography_ch4-menezes_1996.pdf`, `22324838df846601bd46ea112507dcb72904789e95a73798de9140c1ec2e1850` | algorithm titles |
| The same, chapter 14, Efficient Implementation | https://cacr.uwaterloo.ca/hac/about/chap14.pdf | `95-Resources/13-Papers/054-handbook_of_applied_cryptography_ch14-menezes_1996.pdf`, `0f64049b53e41e8399b526bb9d8b6d169e6c36bcc0090833a7e467fcedbf8cf2` | algorithm titles |
| Flajolet, Sedgewick, *Analytic Combinatorics* (2009) | https://algo.inria.fr/flajolet/Publications/book.pdf | `95-Resources/13-Papers/055-analytic_combinatorics-flajolet_2009.pdf`, `f7bc98dfbfbc99716e19a0a0aa6f1750fb62acbf50b58d65731aabf161dccace` | contents |
| Navarro, *Compact Data Structures* (2016), contents and list of algorithms | https://users.dcc.uchile.cl/~gnavarro/CDSbook/toc.pdf | `95-Resources/13-Papers/056-compact_data_structures_contents-navarro_2016.pdf`, `7f1e97d2f8016b1472af846fb5440a97a5230a459c5f77f000fb560e49f0680f` | all |
| Arndt, *Matters Computational* (2010), contents | https://www.jjj.de/fxt/fxtbook-toc.txt | `95-Resources/13-Papers/057-matters_computational_contents-arndt_2010.txt`, `5375ef70ab151ecc8c075ecefc7f4221bc31df30d2fafd74e54005d38cf8da99` | all |
| LibreOJ template problems (tag 模板), list and 12 statements from its API | https://loj.ac/ | `95-Resources/14-Web Pages/032-libreoj-template_problems.zip`, `083252ed52066960af92781caf63e9e86e93c67953d0d86eee254a3113a12026` | all |
| Luogu template problem list (keyword 模板, six pages) and tag map | https://www.luogu.com.cn/problem/list?keyword=%E6%A8%A1%E6%9D%BF&type=P | `95-Resources/14-Web Pages/033-luogu-template_problems.zip`, `88ef869fed4730a80753486391ac1c08815c03c837d609ec9b7fb3e2d4702cc5` | all |
| Aizu Online Judge course problem lists (ALDS1, DSL, GRL, CGL, NTL, DPL, ITP2) | https://onlinejudge.u-aizu.ac.jp/api/courses | `95-Resources/14-Web Pages/034-aizu-courses.zip`, `c69e1a6ec0f893a52a4a623e5ecaa4fb97f572613e2ce8656114fabdea5ad06c` | all |
| *Competitive Programming 4*, Methods to Solve, chapters 2–9 | https://cpbook.net/methodstosolve | `95-Resources/14-Web Pages/035-cpbook-methods_to_solve.zip`, `ecd9fe08441917c6ecede46381fceca2ed61a0cb3853efb22473c714575f52dc` | section labels and hints |
| CodeChef problem tags and Learn catalog | https://www.codechef.com/get/tags/problems | `95-Resources/14-Web Pages/036-codechef-problem_tags.zip`, `463eaf17423ee1a1f84f6aecd59fb43d58d5424ea86c3d1e4b29862801888065` | all |
| Rosetta Code, Category:Programming Tasks (1,363 titles) | https://rosettacode.org/wiki/Category:Programming_Tasks | `95-Resources/14-Web Pages/037-rosetta_code-programming_tasks.zip`, `22fea9b09cd1b90f8b4009b93c56333fce48e1db8308c2f00b6336c259219d5a` | titles |
| UOJ template problems | https://uoj.ac/problems/template | `95-Resources/14-Web Pages/038-uoj-template_problems.zip`, `d3e93d326d1b8713d1187f552b58c9e51ec94c933ce6f44a26619bb59d2f55be` | all |
| igraph C reference, 14 chapters | https://igraph.org/c/doc/ | `95-Resources/14-Web Pages/039-igraph-c_reference_chapters.zip`, `d44a9bc819caf36014c5edafe3232592249bf231f6eb53ad37290e91df77c731` | function anchors |
| PARI/GP reference, contents and chapter indexes | https://pari.math.u-bordeaux.fr/dochtml/html/toc.html | `95-Resources/14-Web Pages/040-pari_gp-reference_chapters.zip`, `ba9ba01e9da176b22bcc9174d39d9f44b89572f8e58cc8d1ce6c6db2fbecb768` | function names |
| e-maxx article index | http://e-maxx.ru/algo/ | `95-Resources/14-Web Pages/041-emaxx-article_index.html`, `2d9a1cb8c077c99fe3f80ce6d0843093b29e973344a626b595b2c8e08973a921` | all |
| Codeforces blog 57282, catalog of tutorials, through the API | https://codeforces.com/blog/entry/57282 | `95-Resources/14-Web Pages/042-codeforces_blog_57282-tutorial_catalog.json`, `9e1277faa63d1474510073b0e3c07fb491410d471f4f1015f154136d77fed949` | all |
| solved.ac tag list (BOJ taxonomy), saved page 1 of 8 | https://solved.ac/problems/tags | `95-Resources/14-Web Pages/043-solved_ac-tags1.pdf`, `17cd3a8b233d173ab333f5b01f365d47f07359ab2b833e04df787130a19bfeb1` | all 3 pages |
| solved.ac tag list (BOJ taxonomy), saved page 2 of 8 | https://solved.ac/problems/tags | `95-Resources/14-Web Pages/043-solved_ac-tags2.pdf`, `3c17773b651cff78b72d026c67701a4cddc650e8e9f48ed2c1aa8dc10d8a8ec3` | all 3 pages |
| solved.ac tag list (BOJ taxonomy), saved page 3 of 8 | https://solved.ac/problems/tags | `95-Resources/14-Web Pages/043-solved_ac-tags3.pdf`, `aa34583704f083c5b4657804fc8277c231144ce5a2ae50d99eb7d8382c4401b1` | all 3 pages |
| solved.ac tag list (BOJ taxonomy), saved page 4 of 8 | https://solved.ac/problems/tags | `95-Resources/14-Web Pages/043-solved_ac-tags4.pdf`, `f5654630b7aa29a26e1b6d464c93dd78baa921bcce4cfa3030301f8b4cf86a15` | all 3 pages |
| solved.ac tag list (BOJ taxonomy), saved page 5 of 8 | https://solved.ac/problems/tags | `95-Resources/14-Web Pages/043-solved_ac-tags5.pdf`, `9ca2e87f6a58ae7dc4fa43f27a19373a1e6ff4c6b4aed1281957d53b1e17b603` | all 3 pages |
| solved.ac tag list (BOJ taxonomy), saved page 6 of 8 | https://solved.ac/problems/tags | `95-Resources/14-Web Pages/043-solved_ac-tags6.pdf`, `bb27609da85c0e06702989d358bfafb5fc035f0ecb849dddada5ad401c96d276` | all 3 pages |
| solved.ac tag list (BOJ taxonomy), saved page 7 of 8 | https://solved.ac/problems/tags | `95-Resources/14-Web Pages/043-solved_ac-tags7.pdf`, `3baec8f2d7d87a45946be72cd7660959ea53fd827aefab50f7169be7b2290847` | all 3 pages |
| solved.ac tag list (BOJ taxonomy), saved page 8 of 8 | https://solved.ac/problems/tags | `95-Resources/14-Web Pages/043-solved_ac-tags8.pdf`, `2c6eb48d4f67693069b97b978430573d7f2253f9d4f96cb1bc4f1cedeed32301` | all 2 pages |
| QOJ template problems (tag 模板题), saved page | https://qoj.ac/problems?tag=%E6%A8%A1%E6%9D%BF%E9%A2%98 | `95-Resources/14-Web Pages/044-qoj-template_problems.pdf`, `8f6c22b8936b11f186616072473e04685a1381cf122552708280b9dafbc48007` | all |
| SPOJ problem tags, saved page | https://www.spoj.com/problems/tags/ | `95-Resources/14-Web Pages/045-spoj-tags.pdf`, `e5b0559505e4e525d9f579608b49b0ad0b08e33a153df7fa8af0cae2f443ac4b` | all |
| LeetCode problem list with topic tags (4,124 problems, 175 topics), exported | https://leetcode.com/problemset/ | `95-Resources/14-Web Pages/046-leetcode-topic_tags.csv`, `217f814be80bdfbdbfce78979124067c5dc1c5653584208b34497cfe4a66da9e` | the topic tags with counts, all 176 |
| yukicoder tag statistics, saved page (logged in) | https://yukicoder.me/statistics/tags | `95-Resources/14-Web Pages/047-yukicoder-tags.pdf`, `4f741591b247c9ca68f64c37038a59c7f6631cbd3d28cbe9cb217f62d86b3d11` | all |
| CMU 15-451 Algorithm Design and Analysis, lecture list, saved page | https://www.cs.cmu.edu/~15451/ | `95-Resources/14-Web Pages/048-cmu_15451-lectures.pdf`, `aee9b4de62949c2a0859f4d690bff4d3ff70fb23c2874fdc15478b60c7e34cb1` | all |
| CMU 15-750 Graduate Algorithms, lecture list, saved page | https://www.cs.cmu.edu/~15750/ | `95-Resources/14-Web Pages/049-cmu_15750-lectures.pdf`, `09921be346139ba02469fe601f3025094e5a973adf734a07d0e0ca15e58d4316` | all |
| MIT Algorithm Wiki, problem families index, saved page | https://algorithm-wiki.org/families | `95-Resources/14-Web Pages/050-algorithm_wiki-index.pdf`, `41526e1e05e7fab5ddec0a5465dd469ec72da719b7c9d2d5531d252ea8bd07fb` | all |
| Kao (ed.), Encyclopedia of Algorithms, 2nd ed. (2016) | https://link.springer.com/referencework/10.1007/978-1-4939-2864-4 | `95-Resources/13-Papers/058-encyclopedia_of_algorithms-kao_2016.pdf`, `bfb3188a65eb9a9acef7a08aeb7d0db7fc2d192a691b5084daec98db44d3a6bc` | list of entries, pages 2420–2428, and page 70 |
| Warren, Hacker's Delight, 2nd ed. (2012) | https://www.informit.com/store/hackers-delight-9780321842688 | `95-Resources/13-Papers/059-hackers_delight-warren_2012.pdf`, `f89446c3655975af750b964c70bb8fbeb0567a970a62032f9f9139d00f0523c0` | contents, pages 1–20; captions of chapters 8–18 and text of chapters 11–12 (pages 140–420) |
| Ahuja, Magnanti, Orlin, Network Flows: Theory, Algorithms, and Applications (1993) | https://www.pearson.com/en-us/subject-catalog/p/network-flows-theory-algorithms-and-applications/P200000003466 | `95-Resources/13-Papers/060-network_flows-ahuja_1993.pdf`, `c30b7e513ac21fcce27bd8515aa4382fb462e65126d3383255654f5e40e71dac` | contents and preface, pages 1–12 |
| de Berg, Cheong, van Kreveld, Overmars, Computational Geometry: Algorithms and Applications, 3rd ed. (2008) | https://link.springer.com/book/10.1007/978-3-540-77974-2 | `95-Resources/13-Papers/061-computational_geometry-de_berg_2008.pdf`, `dd684baf954099204f61774c78414c10f6cde5e5dc4675fe7cc4ed3a25d3fc35` | contents, pages 9–12; index, pages 376–388 |
| Gusfield, Algorithms on Strings, Trees, and Sequences (1997) | https://doi.org/10.1017/CBO9780511574931 | `95-Resources/13-Papers/062-algorithms_on_strings_trees_sequences-gusfield_1997.pdf`, `733c1f7f17870311fec2e2dc5110c350cb8be723a86e4a44f5f5a3d5221883d7` | contents, pages 3–12 |
| Crochemore, Rytter, Jewels of Stringology (2002) | https://doi.org/10.1142/4838 | `95-Resources/13-Papers/063-jewels_of_stringology-crochemore_2002.pdf`, `6689fa0f7b5f7df8313a4ac278c53dc8ff2c851f60445080a580b324983ba4ee` | contents and preface, pages 1–12 |
| Mehta, Sahni (eds.), Handbook of Data Structures and Applications, 2nd ed. (2018) | https://www.routledge.com/Handbook-of-Data-Structures-and-Applications/Mehta-Sahni/p/book/9780367572006 | `95-Resources/13-Papers/064-handbook_of_data_structures-mehta_2018.pdf`, `09cf76124754672ebd5c4915601cbf874ea65b070f04570b1578c9241044b8ab` | contents, pages 7–11; part contents and chapter outlines of all 67 chapters |
| Knuth, The Art of Computer Programming, Volume 4A: Combinatorial Algorithms, Part 1 (2011) | https://www-cs-faculty.stanford.edu/~knuth/taocp.html | `95-Resources/13-Papers/065-taocp_4a-knuth_2011.pdf`, `71d214a013e75cba9f7cb0d423a03fa8e78c8da86534ff428e93c1d10c332b2d` | contents, page 16; Appendices C and D, pages 845–850; the text of pages 18–530 by OCR |
| Knuth, The Art of Computer Programming, Volume 4B: Combinatorial Algorithms, Part 2 (2022) | https://www-cs-faculty.stanford.edu/~knuth/taocp.html | `95-Resources/13-Papers/066-taocp_4b-knuth_2022.pdf`, `b6182fff846cb19ad49f9cc4a781902a356fec5338aa1b1a38e25cb46e2c08f9` | contents, pages 14–16; the text layer searched for its titled algorithms and Appendices C and D |
| Navarro, Compact Data Structures: A Practical Approach (2016) | https://doi.org/10.1017/CBO9781316588284 | `95-Resources/13-Papers/067-compact_data_structures-navarro_2016.pdf`, `3c53650a9508ed40957eb005694149c28c73ab1441a6110210076b9402fed0d8` | contents and list of algorithms, pages 1–30; pages 52–56, 394–403 and 526–531 |
| O'Rourke, Computational Geometry in C, 2nd ed. (1998) | https://doi.org/10.1017/CBO9780511804120 | `95-Resources/13-Papers/068-computational_geometry_in_c-orourke_1998.pdf`, `07003efb5b738d1333f5a4f28adb74a5ebfca35a7384a26bd28ddab981eef545` | contents and every section heading, pages 1–359 |
| Crandall, Pomerance, Prime Numbers: A Computational Perspective, 2nd ed. (2005) | https://link.springer.com/book/10.1007/0-387-28979-8 | `95-Resources/13-Papers/069-prime_numbers-crandall_2005.pdf`, `26d7e47497304958236f104e5098679161039d2171e498fee472ccc3a8e9625c` | preface and contents, pages 1–15; the heading of every numbered algorithm |
| Goodman, O'Rourke, Tóth (eds.), Handbook of Discrete and Computational Geometry, 3rd ed. (2017) | https://www.csun.edu/~ctoth/Handbook/HDCG3.html | `95-Resources/13-Papers/070-handbook_of_discrete_computational_geometry-goodman_2017.pdf`, `be5404858ea197dc66892c1790a3a32e73102d2d66936577cbb092d18d9627da` | contents, pages 10–12; all 565 section headings, table captions and the algorithmic result tables |
| Schrijver, Combinatorial Optimization: Polyhedra and Efficiency (2003) | https://link.springer.com/book/9783540443896 | `95-Resources/13-Papers/071-combinatorial_optimization-schrijver_2003.pdf`, `4f25e8ff85876b3f66b68d0b70b7e796a58e0fcc3d548fa58ed5e8751f317bb5` | contents of volumes A–C, pages 9–38 |
| Cormen, Leiserson, Rivest, Stein, Introduction to Algorithms, 4th ed. (2022) | https://mitpress.mit.edu/9780262046305/introduction-to-algorithms/ | `95-Resources/13-Papers/072-introduction_to_algorithms-cormen_2022.pdf`, `521e1859f5d88ee920270e35e6efd5e4233a8daa047a13a7d1c31978e4ddb52b` | contents, pages 1–25 |
| Laaksonen, Guide to Competitive Programming: Learning and Improving Algorithms through Contests, 3rd ed. (2024) | https://doi.org/10.1007/978-3-031-61794-2 | `95-Resources/13-Papers/073-guide_to_competitive_programming-laaksonen_2024.pdf`, `373df107ab50d807f760fcc435746d1e2cf491bad7fd47f37a994adfd6f1c52e` | contents, pages 8–20, and page 231 |

## Limits

- Not opened: the DMOJ and VNOI sites (403; VNOI through its GitHub source). The pages and books the agents could not open on 2026-11-27 (solved.ac, SPOJ, QOJ, CMU 15-451 and 15-750, the *Encyclopedia*, the Algorithm Wiki, which now lives at https://algorithm-wiki.org/, and the books below) were placed by the user and read on 2026-11-28.
- Read in part: the last 118k characters of Wikipedia's List of algorithms, the tails of the emthrm and beet-aizu sites, PARI chapters on elliptic curves, number fields and modular forms, SageMath's per-module lists, 21 of the NOI.PH notes, the GEOS page beyond 100k characters and six SHOHAG files whose meaning was not opened. On 2026-11-28: of the *Encyclopedia*, only its alphabetical list of 633 entries; of each book, its full contents (with HDCG's section headings and result tables, Crandall–Pomerance's numbered algorithms, TAOCP 4A's text by OCR, Navarro's list of algorithms and de Berg's index), not its body. The placed O'Rourke stops at page 350, without its bibliography and index.
- Samples, not full reads: each catalog was sampled, so the next completeness sweep still reads each one in full; absence was checked against inventories, notes and the ledger, not against code.
- The paywalled books are filed whole since 2026-11-28 (Sources), so a sweep reads their bodies where an operation needs them: AMO, CLRS, CRANDALL, DEBERG, EOA, GUSFIELD, HACKERS, JEWELS, MEHTASAHNI, NAVARRO, OROURKE, SCHRIJVER, TAOCP.

## History

- 2026-11-27: survey at the user's request (user decision 2026-11-27): five read-only agents over judges, curricula, GitHub libraries, books, courses and solver software; 63 catalogs (61 new keys, BOOST's Geometry and WIKI's two lists) and 70 keys (the 61 and 9 without a catalog) added to `09-sources.md`.
- 2026-11-27: candidates applied (user decision 2026-11-27): one triage and one verifying agent per folder; 328 items added to 141 rows, 46 in 15 new rows (P291–P305), 21 left out, 22 already present, 6 speed notes; 16 verified rows reopened as `partial` with follow-ups on 6 verified packages.
- 2026-11-28: the 24 sources the user placed sampled and applied (user decision 2026-11-28): two finders, one triage and one verifying agent per folder; 524 items added to 169 rows, 145 in 27 new rows (P306–P332) and 61 in 10 Graphs rows planned with three-digit prefixes, 33 left out, 67 already present, 23 speed notes; 7 verified rows reopened as `partial` with follow-ups on 6 verified packages; 9 catalogs (SPOJ and YUKI, keys already, and 7 new keys) added to `09-sources.md`.
