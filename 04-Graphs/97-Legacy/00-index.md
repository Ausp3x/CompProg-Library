# Local Legacy References

These excerpts are kept unchanged for future migration and audit. The `.cpp` files are reference text, not standalone programs or active library dependencies; aggregates and notebook discovery exclude this folder. Names, comments (including historical TESTED labels), indexing, dependencies and known defects are preserved. Read only the excerpt you need. The complete originals remain in `../../OLD`, and [the extraction map](../../00-Guidelines/Ledgers/monolith-map.json) records exact source ranges and hashes.

| File | Symbols | Why retained as a reference |
|---|---|---|
| [01-floydwarshall.cpp](01-floydwarshall.cpp) | floydWarshall | Informs row 12. The undefined `sze(d)` prevents standalone compilation, and the free function lacks `inline` for header linkage. |
| [03-subtree_queries.cpp](03-subtree_queries.cpp) | EulerTourTree | Informs row 14. The whole struct is commented out in the source. It is a static Euler subtree flattening with a range-query adapter, not a dynamic Euler-tour tree, and it uses the obsolete non-template `SegTree` with an incompatible constructor and method interface. Its historical TESTED comment is not current verification. |
