# Local legacy references

Unchanged excerpts for future migration/audit. These `.cpp` files are reference text, not standalone programs or active library dependencies; aggregates and notebook discovery exclude this folder. Names, comments (including historical TESTED labels), indexing, dependencies and known defects are preserved. Read only the relevant excerpt. The complete originals remain in `../../OLD`; [the extraction map](../../00-Guidelines/15-monolith-map.json) records exact source ranges and hashes.

| File | Symbols | Why retained as a reference |
|---|---|---|
| [01-transform_algorithms.cpp](01-transform_algorithms.cpp) | FastConv | Equivalent transform family already has an active migrated header; preserve original formatting/body and dependency context for future comparison. |
| [02-modfac.cpp](02-modfac.cpp) | ModFac (commented out) | Original disabled block retained verbatim. Uses old mint .inv()/.pow() API, incompatible with current modint; active combinatorics remains canonical. |
