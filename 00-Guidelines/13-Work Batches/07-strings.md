# 07-Strings implementation batches

Use [the common task prompt](../../01-prompts.md) and the [folder inventory](<../../07-Strings/00-index.md>). Read only selected rows and their required contracts. IDs remain stable across renumbering; filenames below are exact. Every batch owns implementation, research, tests and appropriate benchmarks. Size is relative effort, not a time promise. XL work uses explicit checkpoints and a handoff if needed.

Prerequisites below are conservative API ordering. The full inventory may require an additional backend for a selected variant; inspect direct dependencies before coding. Optional alternatives need not force every backend. The bottom-of-prompts checklist supplies the default session order.

| ID | Owned target filenames | Size | Prerequisites | Focus |
|---|---|---|---|---|
| ST01 | `01-prefixfunction.hpp`, `02-z.hpp` | S | C01 | KMP prefix function and Z. |
| ST02 | `03-stringhash.hpp`, `04-trie.hpp`, `05-manacher.hpp` | M | C01, ST01 | String hash, trie and Manacher. |
| ST03 | `06-aho.hpp`, `07-suffixarray.hpp`, `08-palindrome_queries.hpp` | L | C01, ST01, ST02 | Aho, suffix array and palindrome queries; ST01–ST02. |
| ST04 | `10-suffixautomaton.hpp` | L | C01, ST01 | Suffix automaton, counts and online append. |
| ST05 | `11-suffixtree.hpp` | XL | C01, ST01, ST03 | Offline suffix-tree topology from suffix array/LCP, edge labels and substring loci. ST12 owns Ukkonen construction. |
| ST06 | `12-sais.hpp` | L | C01, ST01, ST03 | SA-IS; ST03 for differential checks. |
| ST07 | `13-palindromictree.hpp`, `14-lyndon.hpp`, `15-minrotation.hpp` | L | C01, ST01 | Eertree, Lyndon and minimal rotation. |
| ST08 | `16-string_matching.hpp`, `17-bitap.hpp` | L | C01, ST01 | Exact single-pattern matching and Bitap. Edit distance and LCS have separate owners after the scope split. |
| ST09 | `22-lexicographic_queries.hpp`, `23-dynamicstringhash.hpp` | L | C01, ST01, ST02, ST03 | Lexicographic queries and dynamic string hashes reuse static hash/suffix APIs; choose existing range/sequence-tree engines for the selected update model. Regular-language automata have a separate owner. |
| ST10 | `26-runs.hpp` | XL | C01, ST01 | Runs and tandem repeats with theorem-level evidence. |
| ST11 | `27-string_periodicity.hpp`, `28-multiple_string.hpp` | L | C01, ST01, ST03, ST04, ST05, ST06 | Periodicity and multiple-string indexing, including substring shortest-border/period queries with explicit preprocessing/query costs. |
| ST12 | `33-onlinesuffixtree.hpp` | XL | C01, ST01, ST05 | Online suffix tree; ST05. |
| ST13 | `34-dynamicaho.hpp` | L | C01, ST01, ST03, ST06 | Dynamic Aho updates/rebuilding; ST03. Dynamic suffix-array research has a separate owner. |
| ST14 | `36-compressed_text_index.hpp` | XL | C01, DS17, ST01, ST06, ST23 | Compressed FM-index and locate sampling reuse canonical BWT and rank/select engines. |
| ST15 | `37-grammar_compression.hpp` | XL | C01, ST01 | Grammar/SLP compression with operational and size contracts; LZ and approximate matching have separate owners. |
| ST16 | `39-palindrome_advanced.hpp` | L | C01, ST01 | Advanced palindrome structures and range queries. |
| ST17 | `40-stringisomorphism.hpp`, `41-debruijn.hpp` | M | C01, ST01 | String isomorphism and de Bruijn sequences. |
| ST18 | `42-zivlempel.hpp` | L | C01, ST01 | Lempel–Ziv parsing/factorization variants; separate from rare linear matching research. |
| ST19 | `09-runlength.hpp` | S | C01, ST01 | runlength. Complete all selected inventory contracts and variants. |
| ST20 | `20-subsequence_automaton.hpp` | L | C01, ST01, ST02 | subsequence automaton. Complete all selected inventory contracts and variants. trie |
| ST21 | `21-wildcardmatching.hpp` | L | C01, ST01, ST08 | wildcardmatching. Complete all selected inventory contracts and variants. string_matching; Mathematics convolution |
| ST22 | `25-radixtrie.hpp` | L | C01, ST01, ST02 | radixtrie. Complete all selected inventory contracts and variants. trie |
| ST23 | `29-bwt.hpp` | M | C01, ST01, ST06 | bwt. Complete all selected inventory contracts and variants. sais |
| ST24 | `30-onlinez.hpp` | L | C01, ST01 | onlinez. Complete all selected inventory contracts and variants. z |
| ST25 | `31-string_reconstruction.hpp` | L | C01, ST01, ST02, ST03 | string reconstruction. Complete all selected inventory contracts and variants. z; prefixfunction; manacher; suffixarray |
| ST26 | `32-multidimensional_matching.hpp` | L | C01, ST01, ST02, ST03 | multidimensional matching. Complete all selected inventory contracts and variants. stringhash; aho |
| ST27 | `44-dynamic_suffix_automaton.hpp` | XL | C01, ST01, ST04 | dynamic suffix automaton. Complete all selected inventory contracts and variants. suffixautomaton |
| ST28 | `45-dynamicpalindrome.hpp` | XL | C01, ST01, ST07 | dynamicpalindrome. Complete all selected inventory contracts and variants. palindromictree |
| ST29 | `46-rindex.hpp` | XL | C01, ST01, ST14, ST23 | rindex. Complete all selected inventory contracts and variants. compressed_text_index; bwt |
| ST30 | `47-dynamiclce.hpp` | XL | C01, ST01, ST09, ST15 | dynamiclce. Complete all selected inventory contracts and variants. lexicographic_queries; grammar_compression |
| ST31 | `48-semilocallcs.hpp` | XL | C01, ST01, ST34 | semilocallcs. Complete all selected inventory contracts and variants. lcs |
| ST32 | `49-historical_string_matching.hpp` | XL | C01, ST01, ST04, ST08 | historical string matching. Complete all selected inventory contracts and variants. string_matching; bitap; suffixautomaton |
| ST33 | `18-editdistance.hpp` | L | C01, ST01 | editdistance. Complete all selected inventory contracts and variants. |
| ST34 | `19-lcs.hpp` | L | C01, ST01 | lcs. Complete all selected inventory contracts and variants. |
| ST35 | `24-regular_language.hpp` | L | C01, ST01 | regular language. Complete all selected inventory contracts and variants. |
| ST36 | `35-dynamicsuffixarray.hpp` | XL | C01, ST01 | dynamicsuffixarray. Complete all selected inventory contracts and variants. |
| ST37 | `38-approximate_matching.hpp` | L | C01, ST01 | approximate matching. Complete all selected inventory contracts and variants. |
| ST38 | `43-linear_string_algorithms.hpp` | L | C01, ST01 | linear string algorithms. Complete all selected inventory contracts and variants. |

Large single-header families keep one owner but require operation-level checkpoints; parallel workers must not independently rewrite the same header. Historical matching and dynamic/compressed-index research use several internal milestones, not a one-shot implementation promise.
