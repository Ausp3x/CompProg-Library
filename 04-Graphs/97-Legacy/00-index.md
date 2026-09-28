# Local legacy references

Unchanged excerpts for future migration/audit. These `.cpp` files are reference text, not standalone programs or active library dependencies; aggregates and notebook discovery exclude this folder. Names, comments (including historical TESTED labels), indexing, dependencies and known defects are preserved. Read only the relevant excerpt. The complete originals remain in `../../OLD`; [the extraction map](../../00-Guidelines/15-monolith-map.json) records exact source ranges and hashes.

| File | Symbols | Why retained as a reference |
|---|---|---|
| [01-floydwarshall.cpp](01-floydwarshall.cpp) | floydWarshall | Undefined sze(d) prevents standalone compilation; free function lacks inline for header linkage. Kept unchanged as a reference; not an active header. |
| [02-bipartite.cpp](02-bipartite.cpp) | isBipartite | Free function lacks inline: unchanged inclusion from multiple translation units violates header linkage requirements. Compiles as a source snippet; kept unchanged as a reference, not an active header. |
| [03-subtree_queries.cpp](03-subtree_queries.cpp) | EulerTourTree | Entire struct is commented out in the source. Static Euler subtree flattening plus range-query adapter, not a dynamic Euler-tour tree. Refers to obsolete non-template SegTree and its incompatible constructor/method interface. Historical TESTED comment is preserved, not accepted as current verification. |
