# Concept Search — Evidence (P263)

Scope (P263, support batch SUP24): `grab --find WORDS`, and the inventory rows grab lists below an unknown name's spelling suggestions. Both are implemented in [`_11_search.py`](<../../09-Contest Testing/_11_search.py>), with the Chinese table [`_11_search_t2s.tsv`](<../../09-Contest Testing/_11_search_t2s.tsv>) derived from OpenCC ([THIRD-PARTY.md](../../THIRD-PARTY.md)), and wired into [`10-grab.py`](<../../09-Contest Testing/10-grab.py>): the search is part of grab, not a new tool (user decision 2026-11-06). The Operations-cell parser (`cells`, `split`, `unqualified`, `depths`, `operations`) moved here from [`05-extraction.py`](../05-extraction.py), which imports it, so the inventory has one parser. Rules: [08-contest-tools.md](../../00-Guidelines/08-contest-tools.md) (Extraction paragraph). Extraction itself: [00-extraction.md](00-extraction.md).

## Contracts

### Documents

- One document per inventory row of `01-Core` … `08-Python`: the rows of `0[1-8]-*/00-index.md` whose first cell is a `.hpp` or `.py` code span. Planned rows, `08-Python` modules and the template and debug headers are included (user decision 2026-11-07 for the last). `show` marks a module with `Python`, and a header the contest template already holds (grab's `EXCLUDED`) with `in template`, after its status.
- Tier: the row's current `## Basic|Advanced|Esoteric` section. A `01-Core` row has no tier section; it is `Basic` when its Basic cell starts with the word `yes` (`yes until mini exists`, and `yes when implemented` for a planned mini), else `Advanced` (user decision 2026-11-07). Status: the first of `verified`, `partial`, `existing-unverified`, `legacy-reference`, `planned` that the Status cell starts with.
- Fields:
  - `stem`: the file stem without `NN-`/`_NN_` (`bare`), split at `_`. A word of it whose stem is no known word, but which splits into two or more pieces that are known words or stop words by their stems, also yields the stems of its pieces other than stop words. Each piece has `PART` (2) letters or more, and at most one is shorter than 3. The split takes the fewest pieces; on a tie, one without a short piece; then the longest pieces from the end (`twosat` → `two sat`, not `twos at`; `disjointsparsetable` → `disjoint sparse table`, not `disjoints parse table`). So `segmenttree` → `segment tree`, `mergesorttree` → `merge sort tree` and `meetinthemiddle` → `meet middle`, while `rindex` stays whole, not `ri nd ex`. A known word is a token of `PART` letters or more in any row's `ops`, `qual` or `aliases`.
  - `ident`: the stem without `_`, and every grab name's `::` parts whole except `operator…` parts, lower case, split at spaces only (`dialdijkstra`).
  - `ops`: the Operations cell without qualifiers. `qual`: the parenthesized qualifiers. `aliases`: below. `folder`: the folder name without its prefix.
  - Every field's text passes `words` and `stem` (Tokens) except `ident`, which is only stemmed: `dialdijkstra` stays one token, and `segmenttrees` would lose its `s`.
- Aliases are read from the row's evidence document only: `<folder>/Docs/<NN-name>.md`, or for a module, once `08-Python` has them, `08-Python/Docs/<module stem>.md` (as `_03_number_theory.md`).
  - What counts: every line of a section whose heading starts with `Aliases`, and every paragraph or list opened by a line `<optional word> Aliases …:` with no backtick before the colon (`Aliases:`, `Catalog aliases:`, `- Aliases (2026-10-19 sweep):`, `- **Aliases elsewhere:**`, `Search aliases (P263, grab --find):`), up to a blank line or a heading. The opening label up to its colon (and a closing `**`) is dropped from each line, so `Search`, `P263` or `grab` index nothing; URLs and link targets are dropped too. `Aliases with \`FIXED\`:` opens nothing.
  - What a package lists there (`/package` step 3): the row's other English names and abbreviations, its OI Wiki page title, and the Chinese names contestants use, in Simplified and mainland usage. A Taiwan or Hong Kong term is listed only when `grab --find` with it does not reach the row through the conversion (Chinese), and then written in Simplified as its regional reading (`普林算法` for `普林演算法`, `连通组件` for `連通元件`). So the documents hold no phrase a query's regional reading replaces, apart from the mainland words `test_repo_chinese` pins.
  - Items: the aliases text, folded and without complexity expressions (`words`), cut at `,` `;` `:` `(` `)` `|` (their fullwidth forms fold to them), `、` and line ends, never inside a code span (`` `flow(s, t, MAX, 0)` `` is one piece, not the items `t` and `0`). A piece cut at a colon labels what follows and is no item (`OI Wiki` in `(OI Wiki: 线段树)`). An item whose tokens equal the query's ranks its row in the alias class (Ranking), so an item names its row whole, once, without a qualifier of its own in parentheses: `BFS（搜索）` had put the traversal row first for `搜索`, "search".
- Names, which a hit offers to grab, in cell order:
  - a type head (capitalized, `::`-qualified, or marked `(alias`, as `iint (alias)`, which a label `aliases:` is not; a head of several words gives its capitalized ones, `A as B and C`) and `Type::member` for its entries. `ctor` names the type; an entry the cell qualifies itself stays as written (`PolyCompanion::fft`); a `std::` entry is a specialization and no name;
  - a camelCase function head alone (`cartesianTree (cmp): parent, root` → `cartesianTree`);
  - the entries of a lower-case label head (`required:`) or of no head.

  Template arguments are stripped; operators stay `Type::operator+=`.

### Tokens

- `words`, in order:
  1. NFKC (fullwidth letters and digits, compatibility ideographs).
  2. `fold`: a Latin letter with diacritics becomes the letter its canonical decomposition begins with (`latin`: `Prüfer` → `prufer`, `Thuật` → `thuat`, `Ǣ` → `æ`). `ø` and `ß`, which have none, stay, and so do Greek and Cyrillic letters.
  3. A complexity expression is dropped, since it names no concept (`unbounded`): `O`, `o`, `Θ` or `Ω` after no letter or digit, then a parenthesized stretch with its nested parentheses, as `O(N+M)` and `O(T(n) log n)`. An unclosed one stays.
  4. Lower case. Identifiers split at `_`, `-`, camelCase, acronym runs (`LCPArray` → `lcp array`) and letter-digit boundaries. A run of Chinese characters is cut from letters and digits and gives character pairs (Chinese).
  5. English stop words (`a an and as at by for from in into is it of on or the to via with`) are dropped, Chinese never.
- `stem`:
  - Plurals first: `ies` → `y`, and `ches/shes/sses/xes/zzes` lose `es` (words over 4 letters); a final `s` goes unless after `s`, `u` or `i` (words over 3 letters).
  - Then `-ing` and a final `e` as the Snowball Porter2 stemmer's steps 1b and 5 treat them. `-ing` goes when the rest holds a vowel; then an `at`, `bl` or `iz` ending or a short word gets its `e` back, and a doubled `bb dd gg mm nn pp rr tt` ending a stem of 4 letters or more loses one letter. A final `e` goes in R2, or in R1 when not after a short syllable. So a word and its `-ing` form share a stem (`sliding`, `slide` → `slide`; `merging`, `merge` → `merg`; `spanning` → `span`; `adding` → `add`; `string` stays).
  - Irregular: `vertices`, `indices`, `matrices`, `series`, `species`, `tries`, `alias`/`aliases`, and `unite`, `unites`, `uniting`, kept apart from `unit`.
  - Among the 3,150 ASCII alphabetic words `words` gives for the header stems, Operations cells, folder names and alias texts, a word and its `s`, `es`, `ies` or `-ing` form share a stem unless they are different words (`prim`/`primes`, `plan`/`planes`, `not`/`notes`, `on`/`ones`, `fac`/`faces`, `str`/`string`) or the form has 3 letters or fewer. Such a form keeps its `s` (`ops`, `ids`, `its`), so `lis` and `sos` stay whole.
- Typo tolerance, for a query token absent from the vocabulary: optimal string alignment distance (an adjacent swap counts 1) at most 0 up to 3 letters, 1 up to `TOLERANCE[1]` (7) letters and 2 beyond (`TOLERANCE`). A match at distance 1 counts 0.85, at distance 2 0.8 (`TYPO`). A token of `PREFIX_MIN` (2) letters or more also matches the tokens it begins, at `PREFIX` (1.0). Each query token takes its best alternative per row.
- Joins: every 2 and 3 adjacent query words, none Chinese, are also joined into extra query tokens (`block cut tree` → `blockcuttree`), matched as typed or within typo tolerance, never as a prefix.

### Chinese

- Pairs: a run of Chinese characters (CJK Unified Ideographs with Extension A, compatibility ideographs after NFKC, planes 2 and 3) gives the overlapping character pairs of its one form (a lone character gives itself); in a query, the new pairs of its regional reading follow. Pairs are tokens like words: weighted, matched exactly (2 letters are below every typo tolerance) and never joined, since they overlap already; adjacent pairs earn the phrase bonus. A lone query character stands for every token holding it, a pair at the prefix factor and itself at 1, each scaled to the character's own idf over the rows holding it in any token. So a common character weighs what its frequency says: `AVL 树` lists `AVLTree` first, and `樹` still finds `树状数组` and `线段树`.
- One form (`simplified`): a run is Traditional when it holds a character outside GB2312 (the mainland character set, by Python's `gb2312` codec) that the character tables convert.
  - A Traditional run goes through OpenCC's conversion as OpenCC's `tw2s` and `hk2s` chain it. It is cut at the longest Traditional-to-Simplified phrase keys (each key is one segment, and each stretch between keys one). Each segment then goes through the Taiwan and Hong Kong variant phrases and variant characters, then through the Traditional-to-Simplified phrases and characters. Within one such group, at each position the longest key of the first dictionary holding a key there is replaced; otherwise the character stays.
  - Then every run has its characters converted by the character table (a character both scripts write, as `乾`, takes one form), and its words replaced by their mainland class's word where `mw` lists one (`想像` and `想象` become one). A longer phrase `mw` lists is a class of its own, so a variant spelling inside it can stay apart (`想像出来` beside `想象出来`).
  - A run without such a character is mainland text, kept as written apart from those two steps, so `查询` stays `查询`.
- Regional reading (`simplified(text, regional)`): any run, Traditional or not, read as Taiwan or Hong Kong text, through the same chain with the regional phrases first in the variant group, as OpenCC's `tw2sp` chains them. Taiwan's and Hong Kong's words for a thing the mainland names otherwise thus become the mainland's (`演算法`, `陣列`, `遞迴`, `字尾` → `算法`, `数组`, `递归`, `后缀`).
  - A query's run adds the new pairs of that reading and never substitutes them. A query in Taiwan words, also in characters both scripts share (`字尾`, `函式`, `字元`), reaches the rows the mainland words name and keeps its own pairs.
  - A query's reading leaves out the regional phrases a row's text holds as written (inside one run of Chinese characters of any field, in its one form), since mainland text uses them as mainland words: `序列`, `外接`, `文件`, `行程`, `宏` and `向量`, which `test_repo_chinese` pins. So `子序列` gains no `子串行` (with `子串`, "substring") and `外接圆` no `外置圆`. A phrase only Traditional writes is in no one form and keeps its reading (`查詢` adds `查找` to `查询`).
  - Documents keep their own pairs only (aliases are mainland text, Documents), so a word counts once. Of the 9,448 mainland runs measured under Fidelity, 69 gain any pair as a query, mostly synonyms (`建立`/`创建`, `新增`/`添加`); that count predates the 2026-11-22 pin of `向量`.
- Table (`_11_search_t2s.tsv`, 6,840 entries, `name<TAB>key<TAB>value` after one header line): six dictionaries from OpenCC's `data/dictionary` at commit `fae2a0a`. Keys and values are all Chinese characters, since the search converts runs of them only. The recipe is under Sources.
  - `ts` (498): the first values of `TSPhrases`.
  - `tc` (4,116): those of `TSCharacters`, under those of the `TSCharactersExt` OpenCC builds from its `@tofu-risk` lines (the first value other than the key); changes only.
  - `vp` (1,166): `HKVariantsRevPhrases` under `TWVariantsRevPhrases`.
  - `vc` (80): the reverse of `HKVariants` under `TWVariants`, as OpenCC's `reverse.py` builds it (an `@reverse-prefer` key first); changes only.
  - `rp` (805): `HKPhrasesRev` under `TWPhrasesRev`.
  - `mw` (175): each word of the classes an `STPhrases` or `TSPhrases` pair links (the mainland side through `tc`, the other through `ts` and `tc`, then `tc`), mapped to its class's word (most often a mainland side, then the shortest, in the basic CJK block, the lowest) where that changes it, plus the identities a class's word needs to stay whole (`富于想像` is no `富于` + `想象`).
- Fidelity, measured against the OpenCC 1.4.2 Python package on the 9,451 distinct runs of Chinese characters in the OI Wiki nav, 20 OI Wiki pages and every Markdown file of `01`–`08` (74,890 characters; 9,448 runs read as mainland):
  - The one form changes no mainland run and is idempotent on all 9,451.
  - Each mainland run was converted by OpenCC's `s2t`, `s2tw` and `s2hk` (8,309, 8,303 and 8,308 results read as Traditional). The one form of every result read as Traditional equals that of OpenCC's own `t2s`, `tw2s` and `hk2s` output, and every result's pairs as a query hold all the run's pairs as a document; the 7 `s2tw` results in characters GB2312 holds (as `么半群` and `意味著`) do so through their regional pairs.
  - With `s2twp` (Taiwan words too; 8,266 read as Traditional), 125 results miss some of the run's pairs. OpenCC's own `tw2sp` loses them as well for 115 (`默认` written `預設` comes back `预设`); the other 10 hold `執行`, which the pinned data's `TWPhrasesRev` keeps where OpenCC 1.4.2 gives `運行`.
  - The regional reading with the whole table differs from OpenCC 1.4.2's `tw2sp` on 32 `s2tw` and 39 `s2twp` results, all at `執行`, `程序` or `行程`, the entries whose first value the pinned data changed.
  - Over OpenCC's own 49,409 mainland phrases (the Simplified sides of `STPhrases` and `TSPhrases` in GB2312 characters), the one form of their `s2t`, `s2hk` and `s2tw` results differs from theirs, and their pairs as a query miss some of the phrase's, on 2, 2 and 4 phrases, each through a variant character (`么凤士多`, `六须鲶`; for `s2tw` also `凑合着`, `显着标志`).

### Ranking

- Score: BM25F with `idf = ln(1 + (N − df + 0.5)/(df + 0.5))`, pseudo frequency `Σ_f w_f · tf_f / (1 − b + b · len_f/avg_f)` and saturation `x(k1 + 1)/(k1 + x)`, summed over the query tokens, each at its best alternative's factor.
  - Phrase bonus: for each two query tokens adjacent in the query's own reading or in its regional reading, the bonus of their best alternatives adjacent in some field of the row, `phrase · (f_a · idf_a + f_b · idf_b)/2`, where `f` is each alternative's factor (a typo, a prefix or a lone character's token counts at its factor, so a lone character's bonus rests on its own idf). `後綴陣列` reads `后缀 缀阵 阵列` and `后缀 缀数 数组`, so it earns the bonus `后缀数组` earns, and no pair across the two readings counts.
  - The sum is multiplied by the row's status factor and tier factor (`STANDING`, `LEVEL`), so finished and simpler rows rise (user request 2026-11-07).
- Order, in classes:
  1. Rows whose stem tokens equal the query's (`montgomery` lists `04-montgomery.hpp` before `ModInt`'s `Context::montgomery`).
  2. Rows with a name equal to the query, or whose last `::` part is, ignoring case, fullwidth forms, Latin diacritics and complexity expressions.
  3. Rows with an alias item whose tokens equal the query's or those of its regional reading (`线段树` and `線段樹` list `03-segmenttree.hpp`, `并查集` lists `01-dsu.hpp`, and `bit` lists `02-fenwick.hpp` before `BitOps`).
  4. For a one-word query, rows with a name holding it as a run of whole words (`bit` in `BitOps`, not `sam` in `same`) or as pieces of 3 letters or more beginning consecutive words of the name (`segtree` in `SegmentTree` and `SegTreeBeats`, `lazyseg` in `LazySegmentTree`). Words with spaces describe a concept, so `binary search` lifts no `parallelBinarySearch`.
  5. The rest with a positive score.

  Only a C++ row that is `verified`, `partial` or `existing-unverified` takes classes 1 to 4: a module, a planned or a legacy-reference row has nothing grab copies, so it ranks by score alone. Classes 1 to 4 order by tier (Basic, Advanced, Esoteric), status (in the order of Documents), score and header path: simplest and most finished first, so `bipartite` lists `bipartiteCheck` before `BipartiteMatching` and `segtree` lists `SegmentTree` first. Class 5 orders by score, tier, status and header path. `-n K` (default 10) keeps the first K; when nothing scores, the output is `no match`.
- Name shown (user decision 2026-11-07): the equal name; else the shortest whole-word or shortened match; else the name sharing the most query idf, a name's words including its entry's parenthesized qualifiers (`miller rabin` shows `isPrime`, whose entry reads `isPrime (deterministic Miller-Rabin)`), then the one with the most words of the header stem, the stem joined into one word counting too (`segtreeBuild` for `01-seg_tree.hpp`), then the first. The header path is shown when no name shares a query word and the row is no own-stem match, so an alias-only match names no unrelated function (`kadane` shows `06-Miscellaneous/08-sequence_algorithms.hpp`, `字尾` shows `07-Strings/12-sais.hpp`), and when a row has no names.
- Output line: the name (`-` when the name shown is the header, which grab cannot take), header, tier and status, each column as wide as its longest entry in the list with no trailing spaces, then `  Python` for a module and `  in template` for a template or debug header. An unknown name's error (libclang path and whole-header fallback) appends `closest rows (name, header, tier, status):` and up to 3 such lines, indented.
- Cost and freshness: every run parses the inventories and evidence documents anew (466 rows, about 0.5 s with the Chinese table) and needs no libclang, cache or index, so a status change, a new row or a new alias line ranks from the next run on. A query takes about 9 ms (median 8 ms, 95th percentile 18 ms, at most 45 ms over the 1,706 queries of the list), and `grab --find shortest path` about 0.55 s in all (2026-11-08, three runs each, the machine otherwise idle). `test_repo_corpus` reads the live inventories, so a renamed or removed corpus target, or a ranking drop below a floor, fails the tester.

### Weights and Cutoffs

The query list is the tester's full mode with `--seed 1` (`corpus` in the tester): the corpus file's queries in file order, then the Library Checker problems in ledger order, then the source `generated`, `typo_variant(query, random.Random(1))` of each `lc` and `cp` query in that order (one random swap, deletion, substitution or insertion inside the query's longest word of 5+ letters, the first letter kept). The objective is the mean over the ten sources `cp oi abbr typo simple py lc generated zhs zht` of (recall@5 + MRR@10)/2. `zh` stays outside, since it checks the OI Wiki titles P263 copied into the aliases. "Even" and "odd" are the queries at even and odd 0-based positions of the list.

- Search (`14-search_tester.py --tune all|even [--start JSON] [--jobs N]`): coordinate descent from the module's constants, or `--start` over them, on the whole list (`all`) or its even half (`even`).
  - Each parameter in turn, in the order below, moves to the best value of its grid with the current value added; the current value is kept on a tie. A best value at the grid's edge grows the grid by the parameter's step (`stem`, `ident` 0.5; `ops` 0.25; `aliases` 1; `folder` 0.3; `k1` 0.8; `phrase` 0.5; `tol2`, `prefix_min`, `part` 1; the rest 0.1) while the grown set stays valid. Rounds repeat until one moves nothing, at most 10.
  - Valid sets (`tune_valid`): every factor in (0, 1], with 0 < `t2` ≤ `t1`, planned ≤ existing-unverified and planned < 1 (the user's request that planned rows rank below implemented ones), and Esoteric ≤ Advanced; `b` in (0, 1]; `qual`, `phrase` and `folder` at least 0, the other weights and `k1` positive; `tol2` above the first length; `prefix_min` and `part` at least 1.
  - Grids: `stem`, `ident` 0.5 1 1.5 2 3; `ops` 0.5 0.75 1 1.5 2; `qual` 0 0.1 0.25 0.5 1; `aliases` 1 1.5 2 2.5 3 4; `folder` 0 0.3 0.6 0.9 1.2; `b` 0.2 0.3 0.5 0.75 0.9; `k1` 0.6 0.9 1.2 1.6 2.4; `phrase` 0 0.5 1 1.5 2; `prefix` 0.45 0.6 0.75 0.9; `t1` 0.5 0.6 0.7 0.85; `t2` 0.3 … 0.7; planned (legacy-reference alike) 0.6 … 0.9; existing-unverified and Advanced 0.8 0.9 1; Esoteric 0.6 … 1; `tol2` 6 … 9; `prefix_min` 2 3 4; `part` 2 3. Verified, partial and Basic stay 1.
  - Output: every step, the values and the module constants; then, with `all`, the per-source results and the floors they give, and with `even`, the odd half's objective of the start, of the tuned values and of the module's constants.
- The first `TOLERANCE` length stays 3 (second run). The list misspells only words of 5+ letters. On 302 extra queries misspelled in a 4-letter word, a first length of 3 scored 0.917 / 0.792 against 0.864 / 0.755 for 4. Those queries are each corpus and Library Checker query holding such a word, in list order, with `random.Random(2)`: a random such word, then a random swap at position 1 or 2 of differing letters, a substitution at 1 to 3 or an insertion at 1 to 4.
- Third run: the `zhs` and `zht` sources joined the objective and the code changed, so the descent ran again from the second run's constants. A partial round and four more moved `folder` 0.6 → 4.5, `qual` 0.25 → 0.5, `aliases` 2 → 1.5, `prefix` 0.75 → 1, `t1` 0.7 → 0.85 and `t2` 0.5 → 0.8 (`stem` went to 2 and back to 1). `--tune all` from these values moved nothing on the final code: 0.885010, against 0.873354 for the second run's constants, and 0.891456 with the four aliases of Decisions, which need no retune. The objective is flat near these values: `folder` 2.4 to 4.8, `prefix` 0.75 to 1, `t2` 0.3 to 0.8, `tol2` 6 to 9, `prefix_min` 1 to 2 and `part` 1 to 3 each stay within 0.0015 of the best. `folder` gains because folder words name a query's area: `python` in the `py` queries (MRR 0.660 at 0.6 against 0.809) and an OI Wiki slug's first part (`graph/`, `string/`, `math/`; `oi` recall 0.814 against 0.841).
- When to retune: not for new rows or alias lines, which every run indexes anew and `test_repo_corpus` (every `01-run.py` mode, quick included) checks against the floors. A floor that a new row breaks is usually restored by an alias line (`/package` step 3) or a corpus mapping. Retune when the ranking code changes, when the corpus gains a source, or when a floor fails that aliases cannot restore: run `--tune all`, then `--tune even --start` the previous constants for the overfitting check, then copy the printed constants to `_11_search.py` and `TUNED`, the printed floors to `FLOORS`, and the results to this section.

| Parameter | Value |
|---|---|
| Field weights | `stem 1, ident 1, ops 1, qual 0.5, aliases 1.5, folder 4.5` |
| `b`, `k1` | 0.5, 1.2 |
| Typo factors (distance 1, 2), prefix | 0.85, 0.8; 1.0 |
| Phrase | 1.0 |
| Status factors `STANDING` | verified 1, partial 1, existing-unverified 1, legacy-reference 0.9, planned 0.9 |
| Tier factors `LEVEL` | Basic 1, Advanced 1, Esoteric 0.9 |
| `TOLERANCE`, `PREFIX_MIN`, `PART` | (3, 7), 2, 2 |

Overfitting check (`--tune even --start` the second run's constants, on the final code): the descent on the even half moved for four rounds and then stopped at `stem 1.5, ident 0.5, ops 1, qual 0.1, aliases 1.5, folder 2.4`, `b` 0.5, `k1` 1.2, phrase 2, prefix 0.6, typo 0.85 and 0.7, existing-unverified and Advanced 0.9, `tol2` 8 (even half 0.8701 → 0.8824). On the odd half, which it never saw, the start scores 0.8765 and those values 0.8822, so the gain carries over to unseen queries. The module's constants, chosen on the whole list and so not held out there, score 0.8876. The halves disagree on most values inside the flat region above, so the constants are one point of a plateau, not a sharp optimum.

### Decisions

Each decision is Claude's unless marked as the user's. The user accepted those of the first two runs on 2026-11-07. Those marked third run (2026-11-07 and 2026-11-08) were Claude's, within the user's instruction for that run ("no more limits"), and were confirmed by the user 2026-11-22.

- Scope of the list: planned rows are included, marked by status and ranked below implemented ones by the status factor (user request). `08-Python` modules are included, marked `Python`, with no factor of their own. Columns: name to grab, header, tier, status. Template and debug rows are included and marked `in template` (user decision).
- A header stem equal to the query ranks first, before exact names: `--find mst` lists `06-mst.hpp` first though no name contains `mst`, and `--find lcs` lists `19-lcs.hpp` before `StringHash::lcs`.
- Name matches are whole-word runs, for one-word queries only. A plain substring had put `DSU::same` first for `sam` and `list` names first for `lis`, and a run inside a longer name had put `parallelBinarySearch` before `binSearch` for `binary search`. A one-word query that shortens a name's consecutive words joins that class (user decision 2026-11-07, third run): `segtree` had listed `SegTreeBeats`, which spells it, before `SegmentTree`, which does not.
- Name matches order simplest and most finished first (user request: `bipartite` lists `bipartiteCheck` first). Concept matches keep the score order, softened by the factors, since a strong Esoteric match should still beat a weak Basic one.
- Every weight, factor and cutoff is the descent's choice (user decision 2026-11-07: "whichever is best"), except the first tolerance length (Weights and Cutoffs). Kept as set, since the corpus cannot measure them (user decision 2026-11-07, which chose tuning for the cutoffs only): grab's 3 concept rows below an unknown name (`CONCEPTS`), floors 0.02 below the results, the corpus mapped by subagents from the inventories rather than by hand, and OI Wiki page slugs in place of article titles.
- A stem splits only into known words, by their stems, with at most one piece shorter than 3 letters, and stop words as pieces that yield nothing (user decision 2026-11-07, third run: with `PART` 2, `46-rindex.hpp` had yielded `ri nd ex`).
- Chinese (user decisions 2026-11-07, third run: "Full Chinese support", then "include traditional chinese"): Simplified and Traditional, in Taiwan and Hong Kong usage too. Claude's choices, third run:
  - OpenCC's dictionaries, with OpenCC's own chains emulated rather than one merged map: a merged map turned `查询` into `查找` and `序列` into `串行` in mainland text.
  - GB2312 to tell a Traditional run.
  - The regional reading added to every run rather than to Traditional runs only. The corpus scores both alike (every source equal except `generated` MRR, 0.811 against 0.812 at the previous constants), but the probes `字尾`, `字首` and `函式` found nothing without it and the suffix-array, trie and multiplicative-function rows with it.
  - Regional pairs added to queries only, documents keeping their own: with regional pairs in the documents too a mainland word counted twice (`序列` matched both the `序列` and the `串行` pairs of every row whose alias held it). On the corpus this is near neutral: 0.8850 against 0.8852 with regional pairs in the documents (`zh` recall 0.708).
  - A lone character weighed by its own idf: at the full or prefix weight of each pair holding it, it outscored the Latin word beside it (`AVL 树`, `PQ 树` and `AA 树` listed tree rows matching `树` first). Without the scaling the objective is also 0.8850, but `zh` falls to 0.698 / 0.679 (from 0.711 / 0.688).
- Fixes from the first independent review of the third run (Claude's):
  - (a) Aliases follow the contract in Documents. A Taiwan word in an alias gave its row every query holding that word (`普林演算法` on the minimum spanning tree put `06-mst.hpp` first for `KMP 演算法`, `Dijkstra 演算法` and `演算法`). So 8 Taiwan or Hong Kong items the conversion reaches were removed (`单调伫列`, `优先伫列`, `互动题`, `尤拉迴路`, `網路流`, `萬用字元`, `快速傅立葉變換`, `么半群`), 20 were rewritten as their regional reading in Simplified (`普林演算法` → `普林算法`, `位元运算` → `比特运算`, `連通元件` → `连通组件`, `最長迴文子字串` → `最长回文子字符串`, …), 8 spelling duplicates of another item were removed (`ST表` beside `ST 表`, `座标压缩` beside `坐标压缩`, …), and the 20 `(OI Wiki X)` titles were written `(OI Wiki: X)`.
  - (b) A query's reading leaves out the regional phrases the documents hold (Chinese). A regional phrase that mainland text uses as a mainland word had turned mainland queries into others (`最长公共子序列` gained `子串`, "substring", from `子串行`). Leaving out every phrase whose one form the documents hold also left out `查詢` and lost `查找` (182 `s2twp` results missing pairs against 125), so the exclusion is now the held phrases alone, found as written in a one-form run (fix (i)).
  - (c) The alias class. The Chinese name of a basic structure listed its variant rows first (`线段树` gave the dynamic segment tree, and `并查集` put `01-dsu.hpp` fourth). An alias equal to the query now ranks its row in a class after exact names, simplest first, like the names (user request 2026-11-07), and before the word-run class, since an alias equal to the whole query names the row more closely than a name holding the word. On the corpus the class is worth 0.0074: 0.8850 against 0.8806 with the alias class after the word-run class (`abbr` 0.875 / 0.848, `simple` recall 0.932) and 0.8776 without it.
  - (d) The phrase bonus takes the adjacent alternatives at their factors, not the largest idf among a group's alternatives whatever matched.
  - (e) The stem split's tie rule, `unites` and `uniting`, and Latin letters outside U+00C0–U+024F.
  - (b) and (d) are invisible to the corpus (0.8850 without the exclusion, 0.8851 with the largest-idf bonus); their probes are in `test_repo_chinese` and `test_phrase`.
- Fixes from the second independent review of the third run (Claude's):
  - (f) An exact name ranks before an alias whatever the tier: an Advanced exact name precedes a Basic alias in `test_alias_class`.
  - (g) The name classes fold the query as `words` does (`fold`); compared as typed, `ＩｎｆＩｎｔ` had listed `InfIntMini` first.
  - (h) 8 P263 items were removed and 1 rewritten, each a duplicate or a title's qualifier: `union find` beside `union-find`; `BFS（搜索）`, `DFS（图论）` and `BFS（图论）` on the traversal and `BFS（图论）` on the shortest-path row, whose qualifiers `搜索` and `图论` became items; `后缀自动机 (SAM)` beside `SAM (suffix automaton, 后缀自动机)`; `discrete logarithm` and `BSGS` beside the section's `DiscreteLogarithm` and `bsgs`; `Z 函数（扩展 KMP）` written `Z 函数`. Found with them, code spans and labels had been cut into items (`` `flow(s, t, MAX, 0)` `` gave `t` and `0`, and `(OI Wiki: X)` gave `OI Wiki` on 25 rows), so a code span now stays whole and a piece before a colon is a label (Documents). (h) raised the objective from 0.8846 (`simple` MRR 0.843 → 0.851).
  - (i) A held phrase is found inside one run, not from pairs adjacent in a field across items (`序列，列埠` had held `序列埠`).
  - (j) Adjacency for the phrase bonus is per reading: a run's regional pairs had followed its own in one sequence, so the bonus joined `阵列` to `缀数` and missed `后缀 缀数`. (j) lowered the objective from 0.8853 (`zht` MRR 0.841 → 0.834). All of the drop is `擴展歐幾里得演算法`, which a pair across the readings had lifted and which now ranks as its Simplified form does, `floorsum` before `mod_arithmetic`; it is a held-out query, so no alias was added.
  - (k) `shortened` tries no piece longer than the word it begins; a 20,000-letter one-word query had taken 8 s. The output is unchanged, so no test can tell.
  - The reviewer found no copying from the held-out sources and no Python 3.11 construct.
- Complexity expressions are dropped from queries and documents alike (Claude, third run, found while reading the remaining misses). `Finding Bridges in O(N+M)` had listed no bridges row in its top 10: its letters `n`, `m` and `o` are rare tokens that 179 field positions hold (`O(NM)`, `O(log n)` in qualifiers and aliases), and `O(NM)` gave the directed MST row the item `nm`. Without them the cp-algorithms titles with a bound find their rows (bridges at 3, articulation points 3 → 2; objective 0.8845 → 0.8850, `cp` recall 0.975 → 0.981).
- Latin letters with diacritics fold to their base letters (third run), since corpus titles and user queries write `Prüfer`, `Möbius` and `Pólya` either way.
- An alias may name a related structure answering the same queries when the library has no row of its own (user decision 2026-11-07); each is marked so in its History line. They are `partition tree` (`划分树`) and `小波树` on the wavelet matrix; `划分树`, `线段树套平衡树` and `树状数组套权值线段树` on the merge sort tree, and `划分树` on the persistent segment tree; `zkw费用流` on min-cost flow; `边分治` on centroid decomposition; `龟速乘` and `光速乘` on modular arithmetic; and `悬线法` on the monotone stack. `cat tree` on the disjoint sparse table is that structure's OI Wiki name.
- Alias lines: each evidence document that needed aliases got one `Search aliases (P263, grab --find):` line (167 documents), at the end of an existing aliases section, after an existing aliases paragraph, or in a new `## Aliases` section before `## History`, each with a History line. They hold English aliases for the misses of the first run's sources `lc`, `cp`, `oi`, `abbr` and `typo` (62 documents); each corpus OI Wiki page's Chinese title, on the rows it maps to that have an evidence document (except five pairs where the page names a helper or another structure of its own row); and Chinese names written by two subagents from the inventories alone, which saw neither the corpus nor the search code (944 added; the first review removed 16 and rewrote 20). Rows without an evidence document (planned or unverified, as `04-Graphs/15-hld.hpp`) get theirs in their own `/package` step 3.
- No alias was written for the `simple`, `py`, `zhs` and `zht` sources, so they measure the ranker on aliases written without them, except four the user asked for on 2026-11-08, after the evaluation, for their misses on rows with an evidence document: `Euler path` on the Eulerian trail, `integer sqrt` on integer roots, `fraction` on rational and `比特集` (the regional reading of `位元集`) on bitset. Those four queries count as seen since and rank first.

## Evaluation

The oracle is independent of the ranker: queries were mapped to rows before any ranking result was seen. The list is the corpus [`14-search_corpus.json`](<../00-Tools/14-search_corpus.json>) plus the Library Checker problems of [`library-checker-coverage.json`](../../00-Guidelines/Ledgers/library-checker-coverage.json) (query: the problem id with `_` as spaces; targets: its `targets`). A query counts as found when any target is among the hits. Every cp-algorithms title was compared byte by byte with the raw navigation page, and every OI Wiki slug and Chinese title with the raw `mkdocs.yml`.

- `cp` and `oi`: a subagent mapped the cp-algorithms titles and OI Wiki slugs from the inventories alone, told not to open the search code or the alias sections, to at most 4 central rows each. Two stretch mappings (`Factoring Exponentiation`, `ds/block-list`) and 8 queries with no row were dropped. In the third run a second subagent, under the same rules, mapped the 230 OI Wiki nav pages the first had not: 159 have no row (site, tool, language and contest pages, and topics the library lacks), 71 were mapped, and 3 stretch mappings were dropped (`math/linear-algebra/vector` to `01-point.hpp`, `graph/node` to `43-implicit_graph.hpp`, `search/opt` to `35-state_space_search.hpp`). So `oi` grew from 233 to 301 pages.
- `zh`: the same 301 pages' Chinese nav titles with the same targets.
- `simple` (59 short everyday queries, each mapped to the simplest row providing it: implemented before planned, lower tier first, then the most elementary) and `py` (30 `python <concept>` queries mapped to the `08-Python` module) were mapped the same way after the user's review; that subagent dropped 9 queries whose simplest row was ambiguous (`hashing`, `modint`, `fft`, …).
- `zhs` (73 everyday Simplified queries by the `simple` rule) and `zht` (74 Traditional queries in Taiwan and Hong Kong usage, half on the `zhs` concepts) were written by a third subagent from the inventories alone, without the search code, its table, the aliases, the corpus or any title list.

Recall@5 / MRR@10 on P263's final code over that list (the tester's full mode, `--seed 1`, 2026-11-08): without the P263 alias lines, without the status and tier factors, and final. Later runs recorded two changes only, with no full table since: `simple` MRR 0.919 (2026-11-10, after alias lines restored its floor) and `py` 1.000 / 0.811 (2026-11-11, after the Python mirror renames). The other figures, the floors and the misses below predate the rows P254, P275 and later packages added.

| Source | Queries | No P263 aliases | No factors | Final |
|---|---|---|---|---|
| `lc` Library Checker | 250 | 0.796 / 0.682 | 0.896 / 0.806 | 0.900 / 0.810 |
| `cp` cp-algorithms titles | 158 | 0.886 / 0.754 | 0.968 / 0.869 | 0.981 / 0.890 |
| `oi` OI Wiki slugs | 301 | 0.764 / 0.637 | 0.827 / 0.687 | 0.837 / 0.708 |
| `abbr` abbreviations | 56 | 0.661 / 0.598 | 0.911 / 0.883 | 0.911 / 0.883 |
| `typo` hand misspellings | 10 | 0.900 / 0.850 | 1.000 / 0.900 | 1.000 / 1.000 |
| `simple` everyday queries | 59 | 0.847 / 0.738 | 1.000 / 0.885 | 1.000 / 0.902 |
| `py` Python modules | 30 | 0.967 / 0.810 | 0.967 / 0.876 | 0.967 / 0.809 |
| `zh` OI Wiki Chinese titles | 301 | 0.193 / 0.173 | 0.711 / 0.692 | 0.711 / 0.688 |
| `zhs` everyday Simplified | 73 | 0.082 / 0.077 | 0.890 / 0.865 | 0.890 / 0.865 |
| `zht` everyday Traditional, Taiwan and Hong Kong | 74 | 0.122 / 0.122 | 0.878 / 0.848 | 0.878 / 0.848 |
| `generated` misspellings (seed 1; stress: seeds 7–9) | 394; 1182 | 0.822 / 0.704 | 0.919 / 0.818 | 0.921 / 0.829; 0.923 / 0.828 |
| Objective (ten sources) | | 0.641 | 0.885 | 0.891 |

How far each column holds out:

- The English P263 alias lines were written for the misses of `lc`, `cp`, `oi`, `abbr` and `typo`, OI Wiki slugs and Library Checker ids among them, so those sources' final columns overstate unseen queries.
- `zh` checks the titles copied into the aliases and stays outside the objective.
- The "No P263 aliases" column measures the ranker on aliases that predate the corpus.
- `simple`, `py`, `zhs` and `zht` had no alias written for them, by writers who never saw them, apart from the four of Decisions, by which their final columns overstate unseen queries (`simple` 0.949 / 0.851 and `zht` 0.865 / 0.834 without them).
- The overfitting check (Weights and Cutoffs) measures the weights.

The factors cost `py` MRR (0.809 against 0.876 without them), since all 30 `py` targets are planned modules, which the status factor ranks below the implemented C++ rows answering the same words (`python sorted list` lists `64-sortedlist.hpp` first). The objective takes the factors for the other sources (0.891 against 0.885).

Floors the tester enforces (0.02 below the final column, rounded down; `typo` and `generated` fixed): lc 0.88/0.79, cp 0.96/0.86, oi 0.81/0.68, abbr 0.89/0.86, typo 0.90/0.90, simple 0.98/0.88, py 0.94/0.78, generated 0.88/0.79, zh 0.69/0.66, zhs 0.87/0.84, zht 0.85/0.82.

Remaining misses (no target in the top 5) on the final code: 187 of the 1,312 queries outside `generated`.

- 155 name only rows without an evidence document, 128 of them planned (87 of these in `zh`; HLD, the Core matrices, offline range queries, DLX, the Euler tour tree, MDST, Sprague–Grundy, digit DP); their own packages add the aliases (Limits and Handoffs).
- 32 are OI Wiki slugs (22) and Library Checker names (10) that name a topic page or a variant more than the row (`ds seg in seg`; `range add range min`, which lists the `RangeAddRangeMin` preset's monoid row first).
- The four held-out misses on documented rows got aliases at the user's request (Decisions): `euler path` was 12th under rows holding `euler` and `path` apart, `integer sqrt` 19th under names saying `isqrt`, `fraction` unranked under a row saying `frac`, and `位元集` unranked, its regional reading `比特集` sharing no pair with `位集`.

## Feature-to-Test Map

Tester: [`14-search_tester.py`](<../00-Tools/14-search_tester.py>). Its oracles, written in the tester:

- hand tables (names, aliases, rows, stem from the Porter2 rules, the Chinese forms from the dictionary lines named) and the Snowball Porter2 document's R1, R2 and short-syllable examples;
- a character scanner over NFKC text telling Chinese characters by their Unicode names, after a scan with a parenthesis count that drops complexity expressions (words);
- the T2S file parsed in the tester, with a recursive longest match per dictionary and a scanning segmenter; Python's `gb2312` codec; an independent table-line count;
- a memoized recursion of the OSA definition (distance, checked on known values) and split enumeration (segment, shortened);
- BM25F recomputed from the formula over hand field lists, and hand-derived orders on fixtures whose scores run against the classes;
- Traditional and Simplified spellings of one query ranking alike on the repository, a scanner cutting alias items around code spans, and a substring scan of the documents' Chinese runs for the regional phrases they hold;
- the canonical decomposition followed by `unicodedata.decomposition` for the Latin letters;
- the corpus.

| Feature | Tests |
|---|---|
| Moved parser (`cells`, `split`, `operations`) | `test_cells_split_operations` (and `13-grab_tester.py`'s harness tests) |
| Names, with their qualifiers (`entries`, `notes`) | `test_names`, `test_entries_notes` |
| Rows: indexes, tiers, Core Basic column (`no` is Advanced), modules and their evidence documents, template and debug rows, status, fields, aliases source | `test_rows_fixture`, `test_repo_rows` |
| Alias forms and exclusions; alias items (`ITEM`, `Row.items`: separators, code spans, labels) | `test_aliases`, `test_alias_items`, `test_rows_fixture` |
| Words (NFKC, Latin diacritics through `latin`, Greek and Cyrillic kept, complexity expressions (`unbounded`: nesting, after a letter, unclosed), splits, stop words, Chinese runs and pairs, regional pairs once each, with the whole or a given regional table), `chinese` | `test_words_table`, `test_words_random`, `test_chinese` |
| Stem (plurals, Porter2 steps 1b and 5 with a vowel just before `-ing`, irregulars with `unites` and `uniting`), `vowels`, `region`, `short` | `test_stem_table`, `test_stem_conflation`, `test_stem_regions` |
| T2S file and `tables` (format, dictionaries, GB2312 and Traditional characters, mainland class words fixed) | `test_t2s_file`, `test_t2s_gb2312`, `test_t2s_mainland_fixed` |
| `traditional`, `segments`, `replace`, `simplified` (one form and regional reading; `rp` before the variants, the first dictionary of a group holding a key, the variant group before the Traditional one, OpenCC's segments) | `test_simplified_table`, `test_simplified_random` |
| Distance (cap + 1 beyond cap), segment (`PART`, stems, at most one short piece, the tie rule: an exact oracle and `twosat`, `disjointsparsetable`), shortened, tolerance (`TOLERANCE`), query stripping (fullwidth, accented, with a complexity expression; an exact name before an alias before a word run, against the scores) | `test_distance_*`, `test_segment_*`, `test_shortened_table`, `test_shortened_random`, `test_tolerance`, `test_strip` |
| Tuned constants equal the table above; `--tune`'s parameters round-trip to them, inside its bounds (`tune_valid`, each bound on both sides) | `test_tuned_constants` |
| BM25F score, default and custom parameters | `test_bm25_one_word` |
| Name classes (own stem, exact name, alias, one-word whole-word runs and shortened names), their gate and their simplest-first order | `test_classes_exact_and_within`, `test_class_gate_and_order`, `test_shortened_class` |
| Tie order, `n`, absent rows | `test_tie_order` |
| Status and tier factors: exact ratios, missing keys, defaults, order flips | `test_priors` |
| Typos, prefixes (`PREFIX_MIN`), the larger of a typo and a prefix factor, joins, stem segmentation (stop words as pieces), phrase bonus (the adjacent alternatives at their factors, adjacency within each reading of a Chinese run) | `test_typo`, `test_prefix`, `test_typo_and_prefix`, `test_joins`, `test_segmentation`, `test_phrase` |
| Chinese ranking: pairs with regional pairs, both scripts alike, the regional phrases a row's text holds in one form left out of a query's reading (in any field, a Traditional spelling by its one form, never across runs or items; against an independent scan), a lone character for its tokens at its own idf, no joins, phrase bonus (also of lone characters), the prefix range past the BMP | `test_chinese_ranking` |
| Alias class: an item equal to the query or its regional reading (Simplified, Traditional, Taiwan words, several words, no reading of a held phrase), after exact names (an Advanced exact name before a Basic alias) and before word runs, gated, simplest first, against the scores | `test_alias_class` |
| Modules outside the name classes | `test_python_modules` |
| Name shown (qualifier match, header for an alias-only match, own-stem exception, joined stem), output line, `Python` and `in template` marks | `test_display_name`, `test_show` |
| CLI `--find` (several words), `no match`, `-n` (default 10), `-n` errors, no index built, `in template` | `test_cli_find` |
| Unknown-name rows: libclang path, fallback, no inventories | `test_cli_unknown_name` |
| Every implemented 01–07 row found by its own stem in the top 3, first unless another C++ row shares the stem (`dsu`, `trie`) | `test_repo_stems` |
| The user's examples on the repository (`bipartite`, `binary search`, `hash` by its order, not its rows' statuses, `debug`) | `test_repo_examples` |
| Traditional and Simplified queries alike, Taiwan words (also in shared characters) the same first row as the mainland ones; the regional phrases the documents hold (pinned, and against an independent scan); canonical rows first for their Chinese names; Taiwan `演算法` queries; `最长公共子序列` without `子串` | `test_repo_chinese` |
| Corpus recall@5 and MRR@10 against the floors, generated typos | `test_repo_corpus` |

## Commands and Results

```bash
python3 '96-Local Testing/00-Tools/14-search_tester.py' --mode quick              # 2026-11-22: PASS, 50 tests
python3 '96-Local Testing/00-Tools/14-search_tester.py' --mode full --seed 1      # 2026-11-09: PASS, 50 tests (2026-11-08: 52 s with the pre-flight, peak 57 MB)
CXX=g++-14 python3 '96-Local Testing/00-Tools/14-search_tester.py' --mode full --seed 2   # 2026-11-08: PASS (no compiler used), 58 s, peak 57 MB
python3 '96-Local Testing/00-Tools/14-search_tester.py' --mode stress --seed 7    # 2026-11-08: PASS, 3 rounds, 76 s, peak 58 MB
python3 '96-Local Testing/00-Tools/14-search_tester.py' --tune all --jobs 8       # 2026-11-08: no move: 0.885010088, the floors above
python3 '96-Local Testing/00-Tools/14-search_tester.py' --tune even --jobs 8 --start '{"qual": 0.25, "aliases": 2, "folder": 0.6, "prefix": 0.75, "t1": 0.7, "t2": 0.5}'
    # 2026-11-08, the second run's constants: even 0.870116 -> 0.882389; odd half start 0.876541, tuned 0.882219, module 0.887576
python3 '96-Local Testing/05-extraction.py' ops --mode quick                      # 2026-11-08: PASS: 8 rows, 159 operations on g++ and g++-14, 0 failures (the moved parser), 189 s
python3 '96-Local Testing/03-consistency.py'                                       # 2026-11-08: no errors
```

The 2026-11-08 runs were in a worktree at d993e52 plus P263's last change, with the four aliases, while P254 rewrote the inventories; that day quick mode passed 50 tests in 18 s (peak 56 MB). A quick run on 2026-11-10 passed (count not recorded). `13-grab_tester.py`, which covers `--find`, records its latest run in [00-extraction.md](00-extraction.md).

Mutation testing covers headers (`05-testing.md`, Mutation Testing), and P263 changed Python tools and documents only, so it has no campaign. Instead, 80 hand mutants of `_11_search.py` (third run) each ran the tester's quick mode with the mutated copy in place of the module, grab's search included. They covered the stem rules and the `-ing` vowel; the stem split (known words, short and stop-word pieces, the tie rule, the `NN-` prefix); joins; lone characters (idf, pairs, prefix range); NFKC and the Latin folding; the conversion (GB2312, dictionaries, order within and between groups, segments, `mw`, the final character pass); regional pairs (queries only, once each, a given table); held phrases; alias items (separators, code spans, labels, folding, complexity); the alias class (place, gate, readings); the phrase bonus (factors, adjacency per reading); complexity expressions (bounds, nesting, unclosed); and `shortened`. The 14 the first pass left alive were closed by tests (a `replace` fixture, `著書` and `醖藉` in `test_simplified_table`, `doing` and `going`, a fixture holding `外，接，嚮量` with `文件` in an Operations cell, an alias row in `test_strip`) or showed dead code, which was removed: the fullwidth separators of `ITEM`, and the test that a held phrase is its own one form, since none of the 582 `rp` keys that differ from their one form occurs in the one form of any key or value of the phrase dictionaries. 75 fail; 5 are equivalent, and two from earlier runs remain equivalent:

| Mutant | Argument |
|---|---|
| The `at`, `bl`, `iz` restoration in `stem` removed | The restored `e` always lies in R1, and step 5 keeps it only after a short syllable starting R1, where the stem is a short word the short-word rule extends anyway |
| `n <= 2` for `n <= 1` in `segment` | States with two short pieces are stored but never read |
| The header's `NN-` digits fed to the stem split | Two digits make no two pieces of `PART` letters |
| A document's regional pairs computed without a regional table | They are its own pairs, dropped as duplicates |
| The bound in `shortened` removed (fix (k)) | A longer piece fails `startswith`, so only the time changes |
| The `stem(w) not in known` guard before `segment` dropped (earlier run) | A known word's fewest-piece split is itself, so `segment` returns nothing; the guard only skips work |
| The `len(t) >= PART` filter of the known set dropped (earlier run) | `segment` never uses a shorter piece; the filter only skips work |

## Sources

- Retrieved 2026-11-07 by the P263 `@researcher` sweep: the Library Checker categories (https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/categories.toml), the KACTL content tree and the ACL document index; abbreviations with a source each (cp-algorithms, OI Wiki, KACTL, ACL, Library Checker, USACO Guide, Nyaan).
- Fetched raw on 2026-11-07 for the titles and slugs compared byte by byte: the cp-algorithms navigation page (https://cp-algorithms.com/navigation.html) and the OI Wiki nav (https://raw.githubusercontent.com/OI-wiki/OI-wiki/master/mkdocs.yml), and 20 of the OI Wiki pages it lists, as Chinese text for the conversion measurements.
- OpenCC (https://github.com/ByVoid/OpenCC), Apache-2.0: `data/dictionary` at commit `fae2a0a911c30764e669a7d8b0591cd8e852934a` (`TSPhrases`, `TSCharacters` with its `@tofu-risk` lines, `STPhrases`, `HKVariants`, `TWVariants`, `HKVariantsRevPhrases`, `TWVariantsRevPhrases`, `HKPhrasesRev`, `TWPhrasesRev`), its `data/scripts/reverse.py` and config chains (`t2s`, `tw2s`, `hk2s`, `tw2sp`); the OpenCC 1.4.2 Python package from PyPI for the fidelity measurements. The repository root at that commit holds no `NOTICE` file (GitHub contents API, 2026-11-08), so Apache-2.0 section 4(d) asks for nothing beyond the license text and the change notice THIRD-PARTY.md and the table's first line give. The Snowball Porter2 stemmer description (https://snowballstem.org/algorithms/english/stemmer.html) for steps 1b and 5.
- T2S recipe: `python3 build_t2s.py DIR OUT '09-Contest Testing'` with `DIR` the dictionary directory above; it imports the search's own `replace`, so the classes are linked as the search converts. The output's sha256 is `3e3b3ff8ee512839589ea8571021d64d8b43c46662ff5b70d7dcfc9ad6c334f1`.

```python
"""Writes _11_search_t2s.tsv from OpenCC's data/dictionary directory: build_t2s.py DIR OUT SEARCH_DIR (where _11_search.py is)."""
import re, sys
from pathlib import Path
src, out = Path(sys.argv[1]), Path(sys.argv[2])
sys.path.insert(0, sys.argv[3])
from _11_search import replace  # the search's own conversion step
HAN = re.compile('[㐀-䶿一-鿿豈-﫿\U00020000-\U0003ffff]+')
BASIC = re.compile('[一-鿿]+')  # the CJK Unified Ideographs block, before the extensions

def entries(name):  # [(key, values, the line before)] and the @reverse-prefer lines of a dictionary
    lines = (src / name).read_text(encoding='utf-8').splitlines()
    return ([(l.split('\t')[0], l.split('\t')[1].split(), lines[i - 1] if i else '') for i, l in enumerate(lines)
             if l.strip() and not l.startswith('#')], [l for l in lines if l.startswith('# @reverse-prefer:')])

def first(name, tofu=False):  # key -> first value; tofu: TSCharactersExt (an @tofu-risk entry's first value other than its key)
    d = {}
    for k, vs, before in entries(name)[0]:
        vs = ([v for v in vs if v != k] if before.startswith('# @tofu-risk:') else []) if tofu else vs
        if vs:
            d.setdefault(k, vs[0])
    return d

def reverse(name):  # data/scripts/reverse.py: value -> its first key, an @reverse-prefer key first
    es, prefs = entries(name)
    d = {}
    for k, vs, _ in es:
        for v in vs:
            d.setdefault(v, []).append(k)
    for line in prefs:
        f = line[len('# @reverse-prefer:'):].split()
        d[f[0]].insert(0, d[f[0]].pop(d[f[0]].index(f[-1])))
    return {k: v[0] for k, v in d.items()}

changes = lambda d: {k: v for k, v in d.items() if k != v}  # a character dictionary's identities change nothing
t = {'ts': first('TSPhrases.txt'),  # OpenCC's t2s, tw2s, tw2sp and hk2s dictionaries, Taiwan's over Hong Kong's
     'tc': changes({**first('TSCharacters.txt'), **first('TSCharacters.txt', tofu=True)}),  # TSCharactersExt first
     'vp': {**first('HKVariantsRevPhrases.txt'), **first('TWVariantsRevPhrases.txt')},
     'vc': changes({**reverse('HKVariants.txt'), **reverse('TWVariants.txt')}),
     'rp': {**first('HKPhrasesRev.txt'), **first('TWPhrasesRev.txt')}}
t = {n: {k: v for k, v in d.items() if HAN.fullmatch(k) and HAN.fullmatch(v)} for n, d in t.items()}  # the search converts
group = lambda *names: [(t[n], max(map(len, t[n]))) for n in names]  # runs of Chinese characters only (no PN結, no ·)
mainland = lambda text: replace(text, group('tc'))  # a mainland run's characters
standard = lambda text: mainland(replace(text, group('ts', 'tc')))  # a standard Traditional word as the search converts it

# mw: classes of words linked by a pair of STPhrases or TSPhrases, both sides as the search converts them (想像 and 想象);
# each member -> the class's word (the mainland side most often, then the shortest, in the basic block, the lowest),
# kept where it changes something, plus the identities a class's word needs to stay whole (富于想像 is no 富于 + 想象)
votes, parent = {}, {}
def find(x):
    while parent.setdefault(x, x) != x:
        x = parent[x]
    return x
for name, side in (('STPhrases.txt', 0), ('TSPhrases.txt', 1)):
    for k, vs, _ in entries(name)[0]:
        for v in vs:
            m, o = (mainland(v), standard(k)) if side else (mainland(k), standard(v))
            if HAN.fullmatch(m) and HAN.fullmatch(o):  # the search converts runs of Chinese characters only
                votes[m] = votes.get(m, 0) + 1
                parent[find(m)] = find(o)
classes = {}
for x in list(parent):
    classes.setdefault(find(x), []).append(x)
word = {x: min(ms, key=lambda m: (-votes.get(m, 0), len(m), not BASIC.fullmatch(m), m)) for ms in classes.values() for x in ms}
t['mw'] = changes(word)
while bad := {v for v in set(word.values()) if replace(v, group('mw')) != v}:
    t['mw'].update((v, v) for v in bad)
with out.open('w', encoding='utf-8', newline='\n') as f:
    f.write('# Third-party: derived from the Open Chinese Convert (OpenCC) dictionaries, https://github.com/ByVoid/OpenCC data/dictionary at '
            'fae2a0a911c30764e669a7d8b0591cd8e852934a, Apache-2.0 (see THIRD-PARTY.md): first values of TSPhrases (ts), TSCharacters '
            'under TSCharactersExt (tc), the Hong Kong under the Taiwan variant phrases (vp), variant characters (vc) and regional '
            'phrases (rp), and mainland word classes (mw), for _11_search.py by the recipe in 96-Local Testing/Docs/00-search.md\n')
    for kind, d in t.items():
        for k in sorted(d):
            f.write(f'{kind}\t{k}\t{d[k]}\n')
```

## Limits and Handoffs

- Rows without an evidence document have no aliases, so only their names, operations and stems find them, and only in English: 299 of the 466 rows, 292 planned and 7 `existing-unverified` (`01-Core/10-matrix.hpp`, `12-bitmatrix.hpp`, `14-sparsematrix.hpp`, `02-Data Structures/23-range_query_offline.hpp`, `04-Graphs/12-all_pairs_shortest_path.hpp`, `13-shortest_path_advanced.hpp`, `15-hld.hpp`). Each package adds them in its `/package` step 3 (accepted by the user on 2026-11-07 as package work).
- The one form merges variant spellings only where OpenCC's phrase pairs link them. Of OpenCC's own 49,409 mainland phrases, 2 to 4 come back from Traditional in another variant (Fidelity: `么凤士多`, `六须鲶`, `凑合着`, `显着标志`), and a longer phrase `mw` lists can keep a variant apart (`想像出来` beside `想象出来`); none is a term of the library's rows or aliases.
- A row found only through an alias, as most Chinese queries find theirs, has no name sharing a word with the query, so it shows `-` and gives grab no name; read the header column instead (accepted by the user on 2026-11-09).

## History

- 2026-11-07: P263 introduced the concept search, its tester and corpus and 62 alias lines, and moved the Operations-cell parser out of `05-extraction.py`; the tester found `distance` returning the true distance above its cap (fixed); `@reviewer` findings fixed; quick, full on g++ and g++-14 and stress passed.
- 2026-11-07: P263 user review: template and debug rows indexed, status and tier factors, name classes gated to grabbable rows and ordered simplest first, the word-run class for one-word queries only, `01-Core` rows outside Basic shown as Advanced; `simple` and `py` corpus sources added; search tester (31 tests) quick, full on g++ and g++-14 and stress, and `13-grab_tester.py` quick and full on g++ and g++-14, passed.
- 2026-11-07: P263 `/finish`: review fixes and the user's answers (status, tier factors and cutoffs retuned on the tester's own query list, alias marks moved to History); search tester (35 tests) and `13-grab_tester.py` quick, full on g++ and g++-14 and stress passed.
- 2026-11-08: P263 third run (user decisions 2026-11-07: "Full Chinese support", "include traditional chinese", no more limits): Chinese queries and aliases through OpenCC's tables with Taiwan and Hong Kong readings, 944 Chinese aliases, the `zh`, `zhs` and `zht` sources, `oi` 233 → 301, the alias class, the stem split into known words, the shortened-name class, Latin diacritics and complexity expressions folded, and a retune with `--tune`; fixes (a)–(k) of two independent reviews and of 80 hand mutants; then, at the user's request, aliases for the four held-out misses and the `simple` and `zht` floors raised with them; search tester (50 tests) quick, full on g++ and g++-14 and stress, `13-grab_tester.py` quick, full on g++ and g++-14 and stress, and `05-extraction.py ops` quick passed.
- 2026-11-09: display only (user request): `show` columns fit their longest entry, status padded, `-` for a header shown as the name; search tester full (50 tests) and grab tester quick passed.
- 2026-11-10: `/finish` P276: the `simple` floor failed (MRR 0.877 < 0.88) after P254 and P275 rows outranked four queries (`matching`, `convex hull trick`, `modular exponentiation`, `longest common subsequence`); alias lines in their rows restored it to 0.919 without a retune (Weights and Cutoffs, When to retune); search tester quick passed.
- 2026-11-11: `/finish` P276 review: the Python mirror renames took `py` to recall 0.900 (`python dinic`, `python binomial`, `python lis`); row qualifiers naming the C++ algorithm or abbreviation (`flow (Dinic)`, `(LIS)`, `combiNR (binomial)`) gave 1.000 / 0.811; corpus and weights unchanged.
- 2026-11-22: after the P013 MI02 landing (user request: fix the other open issues): `test_repo_chinese` failed since P007 added the alias `法向量` (normal vector) to `03-Geometry/Docs/01-point.md`, whose `向量` the Taiwan phrase table reads as `矢量`; `向量`, the mainland word for vector, joins the pinned held phrases, so a mainland query `向量` no longer gains `矢量`; search tester quick passed (50 tests).
