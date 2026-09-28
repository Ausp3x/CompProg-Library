# Local legacy references

Unchanged excerpts for future migration/audit. These `.cpp` files are reference text, not standalone programs or active library dependencies; aggregates and notebook discovery exclude this folder. Names, comments (including historical TESTED labels), indexing, dependencies and known defects are preserved. Read only the relevant excerpt. The complete originals remain in `../../OLD`; [the extraction map](../../00-Guidelines/15-monolith-map.json) records exact source ranges and hashes.

| File | Symbols | Why retained as a reference |
|---|---|---|
| [01-random.cpp](01-random.cpp) | Random, rng | Different from active Random; retains old global rng and lacks current bounds asserts. |
| [02-customhash.cpp](02-customhash.cpp) | CustomHash, safe_unordered_map, safe_unordered_set | Different hash seeding and pair/tuple/string handling from active header; preserve without overwriting it. |
