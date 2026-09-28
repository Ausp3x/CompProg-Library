# Local legacy references

Unchanged excerpts for future migration/audit. These `.cpp` files are reference text, not standalone programs or active library dependencies; aggregates and notebook discovery exclude this folder. Names, comments (including historical TESTED labels), indexing, dependencies and known defects are preserved. Read only the relevant excerpt. The complete originals remain in `../../OLD`; [the extraction map](../../00-Guidelines/15-monolith-map.json) records exact source ranges and hashes.

| File | Symbols | Why retained as a reference |
|---|---|---|
| [01-template.cpp](01-template.cpp) | template aliases and helpers | Active template already exists with different aliases/macros; keep old preamble for interpreting raw excerpts, not as a dependency. |
| [02-debug.cpp](02-debug.cpp) | Debug, debug, trace | Older debug alternative; current Core debug remains canonical. Requires the old template context. |
