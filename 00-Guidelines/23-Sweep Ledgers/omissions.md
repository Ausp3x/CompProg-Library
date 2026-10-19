# Omissions

Completeness gaps found by `/omission-sweep`; `/omission-sweep apply` moves decided items into rows, packages and folder notes.

## Recorded decisions

Decided by the user; binding, and only the user changes them.

- 2026-10-19, suffixtree:3: `11-suffixtree.hpp` adopts a node-returning `lca` (O(1) through the LCP RMQ argmin and a split-to-node map, as ecnerwala `get_split`), rather than composing `lce` and `weightedAncestor`.
- 2026-10-19, `07-suffixarray.hpp`: the default constructor builds through `sais` (generic `T` through `compressAlphabet`); the static builder `SuffixArray<T>::doubling(s, with_rmq)` keeps prefix doubling (a different algorithm and row 12's independent oracle); a small-input dispatch only if the package measures a real crossover.
- 2026-10-19, refinement (P017): symbol ranges with `max - min < max(4n, 256)` (bytes included) are shifted by the minimum in `O(n)` instead of going through `compressAlphabet`; sparse ranges still use it.
- 2026-10-19, suffixtree:2 reversed: `11-suffixtree.hpp` adopts an O(1) `suffixLink` (links precomputed at build time by an offline LCA pass) and `weightedAncestors`, an offline batch in O((n + q)·α(n)); the online `weightedAncestor` stays O(log n).
- 2026-10-19: new Esoteric row in 07-Strings, an O(1)-query weighted-ancestor structure for suffix trees built on `SuffixTree` (GLN 2014, or the simpler CPM 2021 structure if confirmed): build, query, and a benchmark against row 11's jump pointers.
