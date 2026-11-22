# Resources

Offline reading for the library. Open only the relevant reference or chapter; these copies are not verified implementations.

Maintenance: prefer the author's or project's download, keep one edition, keep original contents and notices, and record URL, retrieval date, edition and SHA-256 in [99-sources.json](99-sources.json). Files and folders use `NN-descriptive_name`: a book or paper is `NN-short_title-author_year` (first author's surname; the files of 2026-09-27 omit the year), a saved web page `NN-site_id-short_title` (`07-codeforces_blog_83467-queue_like_undoing.pdf`), all in lowercase `snake_case`; in `13-Papers` and `14-Web Pages`, `NN` follows the order of the 2026-11-22 request, so several pages of one request share it; inside `11-NOI.PH Notes` the number precedes the original title. Check reuse terms before adapting or redistributing material: Agner Fog's PDFs permit local study but prohibit public mirrors. Implementation rules stay in [the guidelines](../00-Guidelines/00-index.md).

Storage: the repository is public, so a file is committed only when its reuse terms permit public redistribution; any other copy (paywalled, library access, a book) stays local in its folder, git-ignored, and so does all of `13-Papers` and `14-Web Pages` (user decision 2026-11-22). Every file here, committed or not, is backed up in the maintainer's private repository CompProg Library Sources (`https://github.com/Ausp3x/CompProg-Library-Sources`), a second git history over this folder that [00-sources.py](00-sources.py) keeps in `.git/sources`: `init` once per clone (it restores the files this clone lacks), `status`, `commit`, and the user pushes. A change that adds or replaces a resource updates it in the same session. The maintainer runs `commit`. A contributor commits only the manifest entry (title, `download_url`, SHA-256, `committed: false`), and `/merge-pr` runs `fetch`, which downloads each missing file and checks its SHA-256, before `commit`; a file that fails is asked of the user through `00-Inbox/`. **Warning:** download a file only from its publisher, its author or another source you are entitled to use, and never commit, upload or share a local-only copy anywhere but the private repository.

`00-Inbox/` (git-ignored) receives the files a session asks the user for because it could not open them; what the citing evidence records: [01-principles.md](../00-Guidelines/01-principles.md), Evidence. A file worth keeping moves into the list below with a manifest entry.

User-provided: [Competitive Programmer's Handbook — Antti Laaksonen](<01-competitive_programmers_handbook-laaksonen.pdf>) and [NOI.PH lecture notes](<11-NOI.PH Notes>) (29 PDFs, the two C++ notes first).

Papers and saved web pages (2026-11-22): [13-Papers](<13-Papers>) and [14-Web Pages](<14-Web Pages>) hold the sources the evidence documents cite, each listed in the manifest with its title, URL and citing documents; most are local-only (Storage).

Downloads (15 PDFs); sources are linked beside each, and edition details and checksums are in the manifest.

| Local reference | Useful for | Source |
|---|---|---|
| [Algorithms — Jeff Erickson](<02-algorithms-erickson.pdf>) | Proofs, dynamic programming, graph algorithms, flows and NP-hardness. | [Author/project](https://jeffe.cs.illinois.edu/teaching/algorithms/) |
| [Open Data Structures (C++) — Pat Morin](<03-open_data_structures-cpp.pdf>) | Data structures, implementation tradeoffs and amortized analysis. | [Author/project](https://opendatastructures.org/) |
| [Mathematics for Computer Science — Lehman, Leighton, Meyer](<04-mathematics_for_computer_science.pdf>) | Proofs, counting, number theory, graphs and probability. | [Author/project](https://courses.csail.mit.edu/6.042/spring18/) |
| [Modern Computer Arithmetic — Brent, Zimmermann](<05-modern_computer_arithmetic.pdf>) | Big-integer multiplication/division, gcd, roots and numerical arithmetic. | [Author/project](https://members.loria.fr/PZimmermann/mca/pub226.html) |
| [KACTL — KTH team notebook](<06-kactl.pdf>) | Compact ICPC implementations and notebook selection/layout reference. | [Author/project](https://github.com/kth-competitive-programming/kactl) |
| [Stanford team notebook (2015–16)](<07-stanford_notebook.pdf>) | Additional contest implementation and notebook reference; older material. | [Author/project](https://github.com/jaehyunp/stanfordacm) |
| [Optimizing software in C++ — Agner Fog](<08-optimizing_cpp.pdf>) | Compiler, cache, scalar and vector optimization; specialized techniques apply to full Core only. | [Author/project](https://www.agner.org/optimize/) |
| [CPU microarchitecture — Agner Fog](<09-microarchitecture.pdf>) | Pipeline, caches, execution ports and hardware behavior for Core optimization. | [Author/project](https://www.agner.org/optimize/) |
| [Instruction tables — Agner Fog](<10-instruction_tables.pdf>) | Instruction latency, throughput and micro-operation reference; hardware-specific measurements. | [Author/project](https://www.agner.org/optimize/) |
| [Fast Fourier Transforms — Jeff Erickson](<12-Advanced Notes/01-fft.pdf>) | FFT derivation and applications. | [Author/project](https://jeffe.cs.illinois.edu/teaching/algorithms/) |
| [Advanced Dynamic Programming — Jeff Erickson](<12-Advanced Notes/02-advanced_dp.pdf>) | Faster dynamic programming and optimization assumptions. | [Author/project](https://jeffe.cs.illinois.edu/teaching/algorithms/) |
| [Matroids — Jeff Erickson](<12-Advanced Notes/03-matroids.pdf>) | Independence systems, exchange properties and greedy correctness. | [Author/project](https://jeffe.cs.illinois.edu/teaching/algorithms/) |
| [Minimum-Cost Flows — Jeff Erickson](<12-Advanced Notes/04-min_cost_flow.pdf>) | Minimum-cost circulation/flow foundations. | [Author/project](https://jeffe.cs.illinois.edu/teaching/algorithms/) |
| [Linear Programming — Jeff Erickson](<12-Advanced Notes/05-linear_programming.pdf>) | LP modeling, geometry and duality. | [Author/project](https://jeffe.cs.illinois.edu/teaching/algorithms/) |
| [Linear Programming Algorithms — Jeff Erickson](<12-Advanced Notes/06-simplex.pdf>) | Simplex and other LP algorithm concepts. | [Author/project](https://jeffe.cs.illinois.edu/teaching/algorithms/) |

Additional online reading: [Algorithms for Modern Hardware — Sergey Slotin](https://en.algorithmica.org/hpc/) and [robust geometric predicates — Jonathan Richard Shewchuk](https://www.cs.cmu.edu/~quake/robust.html). The broader [research source list](../00-Guidelines/09-sources.md) covers maintained libraries, judges, notebooks and multilingual references, and the excluded sources.

The [completeness audit](../00-Guidelines/History/2026-09-27-inventory-audit.md) and the [inspected-source record](../00-Guidelines/Ledgers/research-sources.json) add algebra systems, computational-geometry and graph toolkits, dynamic succinct structures, current multilingual contest libraries and research papers as source categories, and tell catalog inspection from detailed reading.
