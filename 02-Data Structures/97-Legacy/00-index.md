# Local legacy references

Unchanged excerpts for future migration/audit. These `.cpp` files are reference text, not standalone programs or active library dependencies; aggregates and notebook discovery exclude this folder. Names, comments (including historical TESTED labels), indexing, dependencies and known defects are preserved. Read only the relevant excerpt. The complete originals remain in `../../OLD`; [the extraction map](../../00-Guidelines/Ledgers/monolith-map.json) records exact source ranges and hashes.

| File | Symbols | Why retained as a reference |
|---|---|---|
| [01-monset.cpp](01-monset.cpp) | MonSet | Incomplete customization scaffold: states, combine, and map are placeholders. |
| [03-dynsegtree.cpp](03-dynsegtree.cpp) | DynSegTree | max(l, 0LL) conflicts with current Core lng=int64_t on LP64; source used lng=long long. |
| [04-lichao_older.cpp](04-lichao_older.cpp) | LiChaoTree | TODO draft is a complex-vector hull skeleton without queries; distinct from the newer Li Chao implementation. |
| [05-mergesorttree_older.cpp](05-mergesorttree_older.cpp) | MergeSortTree | Meaningfully distinct static fractional-cascading reference; pull reads exhausted merge sides before guards in some expressions. Original TESTED comment is historical, not current verification. |
