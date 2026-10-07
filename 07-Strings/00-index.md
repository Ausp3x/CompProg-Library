# 07 Strings — inventory

Scope: this folder owns string matching, string indexes (prefix/Z, hashes, suffix array/automaton/tree, Aho–Corasick, eertree), string-prefix dictionaries, periodicity, palindromes, subsequence and alignment DP, string compression and the string-specific automata that consume them. Data Structures owns binary integer/XOR tries, sparse tables and succinct rank/select engines; Mathematics owns convolution and combinatorial counting; Miscellaneous owns Huffman coding, digit DP and expression parsing; Graphs owns LCA and Eulerian trails that string adapters reuse. Contracts and ownership rules are in [80-notes.md](docs/notes.md); the source ledger is in [81-sources.md](docs/sources.md).

## Basic

| Header | Operations | Status |
|---|---|---|
| `01-prefixfunction.hpp` | prefixFunction; validPrefixFunction; KmpMatcher: reset, step, matched; kmpOccurrences; prefixOccurrences (from pi); prefixOccurrences (pattern in text); prefixBorders; prefixPeriod; prefixPeriods; prefixAutomaton | verified [90-prefix_z.md](docs/p016-prefix_z.md), [94-p016.md](docs/p016.md) |
| `02-z.hpp` | zFunction; extendedZ; zOccurrences; zBorders; zPeriods; zPeriod; prefixToZ; zToPrefix; validZFunction | verified [90-prefix_z.md](docs/p016-prefix_z.md), [94-p016.md](docs/p016.md) |
| `03-stringhash.hpp` | StringHash<WRAP64>: Digest, defaultBase, randomBase, encode, add, subtract, multiply, build, size, get, reverseGet, power, concat, common, lcp, lcs; StringHash64; StringHash61 (Mersenne 2^61-1 field) | partial [91-stringhash.md](docs/03-stringhash.md), [94-p016.md](docs/p016.md); missing: StringHash61 |
| `04-trie.hpp` | Trie: Node, clear, size, empty, newNode, findNode, count, countPrefix, search, insert, erase, forEach (prefix), forEach (all); TrieDense (fixed-alphabet array transitions): insert, erase, count, countPrefix, search, forEach | partial [92-trie_manacher.md](docs/p016-trie_manacher.md), [94-p016.md](docs/p016.md); missing: TrieDense |
| `05-manacher.hpp` | Manacher: odd, even, size, oddInterval, evenInterval, oddInclusive, evenInclusive | verified [92-trie_manacher.md](docs/p016-trie_manacher.md), [94-p016.md](docs/p016.md) |
| `06-aho.hpp` | AhoCorasick: Node, clear, size, patterns, add, build (sparse), build (dense), sparseStep, step, matchCount, forbidden, safeStep, avoids, suffixAggregate, forEachOutput, forEachMatch, stateCounts, countPatterns, countPositions, countMatches; fromTrie (failure/output links over an external trie) | partial [95-aho.md](docs/06-aho.md), [89-p017.md](docs/p017.md); missing: fromTrie |
| `07-suffixarray.hpp` | SuffixArray<T>: text, sa, rank, lcp, build, buildRmq, size, lce, substringLce, patternRange (vector), patternRange (string_view), longestRepeated; longestCommonSubstring (vector); longestCommonSubstring (string_view) | verified [96-suffixarray.md](docs/07-suffixarray.md), [89-p017.md](docs/p017.md) |
| `08-palindrome_queries.hpp` | isPalindrome (Manacher); maybePalindrome (hash); longestPalindrome | verified [97-palindrome_queries.md](docs/08-palindrome_queries.md), [89-p017.md](docs/p017.md) |
| `09-runlength.hpp` | runLengthEncode; runLengthSize; runLengthDecode; runLengthSpans | verified [93-runlength.md](docs/09-runlength.md), [94-p016.md](docs/p016.md) |

## Advanced

| Header | Operations | Status |
|---|---|---|
| `10-suffixautomaton.hpp` | SuffixAutomaton: Node (len, link, next), extend, build, size, isSubstring, occurrenceCount, firstOccurrence, occurrences, endposSize, minimalLength, distinctSubstrings, distinctSubstringsOnline, totalSubstringLength, kthSubstringDistinct, kthSubstring (with multiplicity), lexicographicWalk, longestCommonSubstring, cyclicShiftOccurrences, suffixLinkTree | planned; CPALG, OI-STR, MASPYPY, YOSUPO |
| `11-suffixtree.hpp` | SuffixTree (offline from SuffixArray): build, root, child, edgeLabel, depth, leafSuffix, locate (explicit/implicit locus), isSubstring, occurrences, weightedAncestor (locus of s[i,j)), maximalRepeats, supermaximalRepeats, distinctSubstrings, dfsOrder | planned; OI-STR, KACTL, MASPYPY |
| `12-sais.hpp` | sais (integer alphabet with upper bound); sais (string_view); saisSentinelFree; lcpArray (Kasai); compressAlphabet | planned; ACL, OI-STR, YOSUPO |
| `13-palindromictree.hpp` | PalindromicTree: Node (len, link, series, next, count), append, build, longestSuffixPalindrome, distinctPalindromes, occurrenceCounts, occurrencesAt, palindromicFactorization, minPalindromePartition (witness), evenPalindromePartition, palindromicLength (per prefix via series links), seriesLink | planned; OI-STR, MASPYPY, YOSUPO |
| `14-lyndon.hpp` | duval; standardFactorization; lyndonArray; longestLyndonPrefix; lyndonTree; isLyndon; lyndonWords (FKM enumeration by length and alphabet) | planned; CPALG, OI-STR, MASPYPY, YOSUPO |
| `15-minrotation.hpp` | minRotation (Booth); minRotation (suffix array); maxRotation; rotationEquivalent; canonicalRotation (tie convention for periodic strings); minCyclicShift | planned; KACTL, CPALG, OI-STR |
| `16-string_matching.hpp` | naiveSearch; rabinKarp; boyerMoore; goodSuffixTable (Z-based); badCharacterTable; horspool; sunday; twoWay (critical factorization); dfaSearch (prefixAutomaton adapter); firstOccurrence; allOccurrences; countOccurrences | planned; LECROQ, CPALG, OI-STR, NYAAN |
| `17-bitap.hpp` | shiftAnd; shiftOr; shiftOrMultiword; patternMasks; bitapHamming (k mismatches); bitapEdit (k edits, Wu–Manber) | planned; LECROQ, OI-STR |
| `18-editdistance.hpp` | levenshtein; levenshteinWitness (alignment); hirschberg; myersBitVector; bandedEditDistance; thresholdEditDistance (early exit); weightedEditDistance (insert/delete/substitute costs); damerauLevenshtein (optimal string alignment); damerauLevenshtein (unrestricted); needlemanWunsch (global alignment); smithWaterman (local alignment); gotoh (affine gap) | planned; LECROQ, OI-STR, MASPYPY |
| `19-lcs.hpp` | lcs; lcsWitness; huntSzymanski; hirschbergLcs; bitsetLcs; myersDiff (shortest edit script); longestPalindromicSubsequence; countPalindromicSubsequences (distinct); countPalindromicSubsequences (multiset); countDistinctSubsequences | planned; OI-STR, MASPYPY |
| `20-subsequence_automaton.hpp` | SubsequenceAutomaton: build, next, prev, isSubsequence, countDistinctSubsequences (modular/saturated), kthDistinctSubsequence, shortestAbsentSubsequence (witness), countSubsequenceOccurrences (pattern as subsequence), isSubsequenceOnline | planned; OI-STR, MASPYPY, NYAAN, YOSUPO |
| `21-wildcardmatching.hpp` | wildcardMatch (convolution; wildcard in pattern, text or both); wildcardMatchDeterministic (alphabet decomposition); globMatch (variable-length star DP); globAutomaton | planned; NYAAN, MASPYPY, EI1333, OI-STR, YOSUPO |
| `22-lexicographic_queries.hpp` | lce; longestCommonSuffix; compareSubstrings; compareSubstringsMany (many strings); DictionaryOfBasicFactors (KMR): build, name, equal, compare; sortSubstrings; kthDistinctSubstring; kthSubstring (with multiplicity); occurrenceRange; occurrenceCount; frequencyStatistics; allPairsLcp; minimalAbsentWords; minimalUniqueSubstrings; shortestUniqueSubstring (position query); lexMinSuffixPerPrefix; lexMaxSuffixPerPrefix; distinctSubstringsPerPrefix | planned; CPALG, MASPYPY, SUISEN, YOSUPO |
| `23-dynamicstringhash.hpp` | PointUpdateHash (Fenwick): set, get, lcp; SequenceHash (sequence tree): insert, erase, split, join, reverse, get; DequeHash: pushFront, pushBack, popFront, popBack, get; HashMonoid (length-aware); mutableLcp | planned; MASPYPY, SUISEN, NYAAN |
| `24-regular_language.hpp` | Nfa: fromRegex (Thompson), epsilonClosure, accepts; Dfa: fromNfa (subset construction), minimize (Hopcroft), complement, intersection, unionWith, concatenate, equivalent, accepts, countAccepted (fixed length, digit-DP bridge); Dafsa: build (sorted dictionary), accepts, rank, unrank | planned; OI-STR, LECROQ |
| `25-radixtrie.hpp` | RadixTrie: insert, erase, count, countPrefix, search, longestPrefixMatch, forEach, split, merge; PersistentRadixTrie: insert, erase, snapshot, count | planned; OI-STR |
| `26-runs.hpp` | runs (Runs theorem via Lyndon roots); runsMainLorentz; squares (distinct); squares (all occurrences); tandemRepeats; primitiveRoot; runsCovering (position query); repetitionsCompressed | planned; RUNS, OI-STR, MASPYPY, NYAAN, SUISEN, YOSUPO |
| `27-string_periodicity.hpp` | borders; periods; primitiveRoot; isPrimitive; fineWilf; borderOccurrenceCounts; periodicPrefixes; isSquareFree; isCubeFree; BorderTree (failure tree): build, lca, longestCommonBorder; substringShortestBorder; substringShortestPeriod; substringPeriods; countUnborderedStrings | planned; CPALG, OI-STR, RUNS, MASPYPY |
| `28-multiple_string.hpp` | GeneralizedSuffixArray: build, documentOf, longestCommonSubstring (k strings), commonSubstringCounts; GeneralizedSuffixAutomaton: extend, documentCounts, longestCommonSubstringAll; GeneralizedSuffixTree: build, documentLeaves; separatorEncode; overlapMatrix; shortestCommonSuperstring (bitmask DP); greedySuperstring | planned; CPALG, OI-STR, MASPYPY, YOSUPO |
| `29-bwt.hpp` | bwt (sentinel); bwt (primary index); inverseBwt; lfMapping; psiMapping; bwtFromSuffixArray; sortCyclicRotations | planned; OI-STR |
| `30-onlinez.hpp` | OnlineZ: append, finalized (newly final indices), query, flush, size | planned; MASPYPY |
| `31-string_reconstruction.hpp` | stringFromPrefixFunction; stringFromZ; stringFromSuffixArray; stringFromSuffixArrayLcp; stringFromManacherOdd; stringFromManacher (odd/even); minimalAlphabet; lexMinWitness; infeasible (status) | planned; MASPYPY, OI-STR |
| `32-multidimensional_matching.hpp` | Hash2D: build, get (rectangle), concatRows, concatCols; match2D (hash); match2DAutomaton (Bird–Baker rows via Aho, columns via KMP); count2D | planned; MASPYPY, NYAAN, HITONANODE |

## Esoteric

| Header | Operations | Status |
|---|---|---|
| `33-onlinesuffixtree.hpp` | UkkonenSuffixTree: append, finalize, locate, isSubstring, occurrences, leafCount, edgeLabel, implicitLeaves, explicitSuffixes | planned; KACTL, OI-STR |
| `34-dynamicaho.hpp` | DynamicAhoCorasick (binary buckets): insert, erase (multiset), countMatches, countPatterns, forEachMatch, rebuild; OnlineAhoCorasick (insert without rebuild): insert, step, matchCount | planned; OI-STR, HITONANODE |
| `35-dynamicsuffixarray.hpp` | SuffixBalancedTree: pushFront, popFront, rank, compare, lcp; AppendSuffixArray (batched rebuild): append, sa, patternRange; EditSuffixArray (insert/erase symbol): insert, erase, sa, rank | planned; OI-STR |
| `36-compressed_text_index.hpp` | FmIndex: build (from bwt), backwardStep, count, locate (sampled suffix array), extract; BidirectionalFmIndex: extendLeft, extendRight, count; CompressedSuffixArray: lookup, access; Xbw: build, subpathSearch, childrenRange | planned; RINDEX, DS17 |
| `37-grammar_compression.hpp` | Slp: build, size, expand, access, substring, matchPattern; rePair; balancedGrammar; slpFromLz77 | planned; DYNAMICSTRINGS, RINDEX |
| `38-approximate_matching.hpp` | hammingDistances (convolution, all alignments); kMismatch (Landau–Vishkin, LCE kangaroo); kMismatchWildcard; boundedEditOccurrences (Landau–Vishkin); qGramFilter | planned; OI-STR, LECROQ |
| `39-palindrome_advanced.hpp` | palindromicLength (per prefix); palindromicCharacteristics (k-palindromes); distinctPalindromesInRange; longestPalindromeInRange; countPalindromesInRange; shortestUniquePalindromes; palindromePairs | planned; OI-STR, MASPYPY |
| `40-stringisomorphism.hpp` | parameterizedMatch (predecessor encoding); orderPreservingMatch; injectiveRenamingMatch; canonicalForm; parikhVector; abelianMatch (Parikh vectors, jumbled) | planned; MASPYPY |
| `41-debruijn.hpp` | deBruijnSequence (FKM via Lyndon words); deBruijnSequence (Eulerian); deBruijnCyclic; deBruijnLinear; universalCycle (feasibility) | planned; CPALG, CSES |
| `42-zivlempel.hpp` | lz77 (overlapping); lz77 (non-overlapping); lz77 (windowed); lz77Decode; lz78; lz78Decode; longestPreviousFactor; lz77FromSuffixArray; lz77Online | planned; OI-STR, MASPYPY |
| `43-linear_string_algorithms.hpp` | criticalFactorization; localPeriod; galilSeiferas; crochemorePartitioning (constant workspace); inPlaceSuffixSort (read-only input, O(1) workspace); sparseSuffixArray (b sampled positions) | planned; RUNS, LECROQ, OI-STR |
| `44-dynamic_suffix_automaton.hpp` | RollbackSuffixAutomaton: extend, rollback, snapshot; PersistentSuffixAutomaton: extend (versioned), state; SlidingWindowSuffixAutomaton: pushBack, popFront, distinctSubstrings | planned; OI-STR |
| `45-dynamicpalindrome.hpp` | DoubleEndedPalindromicTree: pushBack, pushFront, popBack, popFront, distinctPalindromes, longestSuffixPalindrome, longestPrefixPalindrome, count; RollbackPalindromicTree: append, rollback | planned; MASPYPY, YOSUPO |
| `46-rindex.hpp` | RlBwt: build, runs, rank, count; RlFmIndex: backwardStep, count; RIndex: count, locate (O(r) samples), extract | planned; RINDEX |
| `47-dynamiclce.hpp` | DynamicString (locally consistent parsing): concat, split, equal, lce, compare, substring; RecompressionString (deterministic variant): concat, split, equal, lce | planned; DYNAMICSTRINGS |
| `48-semilocallcs.hpp` | prefixSubstringLcs; SemiLocalLcs (seaweed): build, prefixSubstring, substringPrefix, substringString, permutationMatrix; cyclicLcs; cyclicEditDistance | planned; SEMILOCAL, MASPYPY, TKO919, YOSUPO |
| `49-historical_string_matching.hpp` | simon; colussi; galilGiancarlo; apostolicoCrochemore; notSoNaive; turboBoyerMoore; apostolicoGiancarlo; reverseColussi; tunedBoyerMoore; zhuTakaoka; berryRavindran; smith; raita; reverseFactor; turboReverseFactor; forwardDawg; bndm; backwardOracle; orderedAlphabet; optimalMismatch; maximalShift; skipSearch; kmpSkipSearch; alphaSkipSearch | planned; LECROQ |
| `50-combinatorial_words.hpp` | fibonacciWord; thueMorseWord; thueMorseBit; periodDoublingWord; sturmianWord (slope, intercept); christoffelWord; isSquareFree; isOverlapFree | planned; RUNS, OI-STR |

## Notes

- Binary integer/XOR tries, sparse tables, wavelet matrices and succinct rank/select live in Data Structures; `07`, `22`, `36`, `46` and `48` consume them.
- Convolution lives in Mathematics `13-convolution.hpp` (batch MA06); `21`, `32` and `38` require it.
- Huffman/optimal prefix coding, digit DP and expression parsing live in Miscellaneous; `24` only supplies automata to digit DP.
- LCA and Eulerian trails live in Graphs; `27` BorderTree and `41` use them as engines or oracles. Necklace/Lyndon counting belongs to Mathematics generating functions.
- Each callable implementation has one owner: offline suffix tree in `11`, Ukkonen in `33`, two-way in `16`, proofs and rare linear variants in `43`, LZ in `42`, grammars in `37`.
